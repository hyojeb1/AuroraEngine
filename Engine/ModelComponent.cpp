#include "stdafx.h"
#include "ModelComponent.h"

#include "Renderer.h"
#include "ResourceManager.h"
#include "GameObjectBase.h"
#include "CameraComponent.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(ModelComponent)

vector<ModelComponent*> ModelComponent::s_modelComponents = {};

GameObjectBase* ModelComponent::CheckCollision(const XMVECTOR& origin, const XMVECTOR& direction)
{
	GameObjectBase* collidedObject = nullptr;
	float closestDistance = numeric_limits<float>::max();
	XMVECTOR dirNormalized = XMVector3Normalize(direction);

	for (ModelComponent* modelComp : s_modelComponents)
	{
		float dist = 0.0f;
		if (modelComp->m_transformedbBoundingBox.Intersects(origin, dirNormalized, dist))
		{
			if (dist < closestDistance)
			{
				closestDistance = dist;
				collidedObject = modelComp->m_owner;
			}
		}
	}

	return collidedObject;
}

void ModelComponent::Initialize()
{
	m_deviceContext = Renderer::GetInstance().GetDeviceContext();
	m_worldNormalData = &m_owner->GetWorldNormalBuffer();

	ResourceManager& resourceManager = ResourceManager::GetInstance();

	CreateShaders();

	// 모델 및 재질 로드
	for (const auto& [modelName, materialName] : m_modelAndMaterialFileNames)
	{
		const Model* model = resourceManager.LoadModel(modelName);
		Material material = resourceManager.LoadMaterial(materialName);
		m_modelsAndMaterials.emplace_back(model, material);
	}
	UpdateBoundingBox();

	m_worldNormalConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::WorldNormal);
	m_materialFactorConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::MaterialFactor);
	m_dissolveConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::Dissolve);

	s_modelComponents.emplace_back(this);
}

void ModelComponent::Update()
{
	m_boundingBox.Transform(m_transformedbBoundingBox, m_owner->GetWorldMatrix());
}

void ModelComponent::Render()
{
	Renderer& renderer = Renderer::GetInstance();
	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
	XMVECTOR boxCenter = XMLoadFloat3(&m_transformedbBoundingBox.Center);
	XMVECTOR boxExtents = XMLoadFloat3(&m_transformedbBoundingBox.Extents);

	const XMVECTOR& sortPoint = renderer.GetRenderSortPoint();

	// 일반 렌더링
	renderer.RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
		// 카메라로부터의 거리
		XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
		[&]()
		{
			// 프러스텀 컬링
			if (m_transformedbBoundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			// 상수 버퍼 업데이트
			m_deviceContext->UpdateSubresource(m_worldNormalConstantBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);
			m_deviceContext->UpdateSubresource(m_dissolveConstantBuffer.Get(), 0, nullptr, &m_dissolveData, 0, 0);

			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

			for (const auto& [model, material] : m_modelsAndMaterials)
			{
				// 재질 텍스처 셰이더에 설정
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::BaseColor), 1, material.baseColorTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::ORM), 1, material.ORMTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Normal), 1, material.normalTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Emission), 1, material.emissionTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Noise), 1, resourceManager.GetNoise(m_selectedNoiseIndex).GetAddressOf());

				// 재질 팩터 설정
				m_deviceContext->UpdateSubresource(m_materialFactorConstantBuffer.Get(), 0, nullptr, &material.m_materialFactor, 0, 0);

				for (const Mesh& mesh : model->meshes)
				{
					resourceManager.SetPrimitiveTopology(mesh.topology);
					// 메쉬 버퍼 설정
					constexpr UINT STRIDE = sizeof(Vertex);
					constexpr UINT OFFSET = 0;

					m_deviceContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &STRIDE, &OFFSET);
					m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
					m_deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
				}
			}
		}
	);

	// 섀도우 맵 렌더링
	renderer.RENDER_FUNCTION(RenderStage::DirectionalLightShadow, m_blendState).emplace_back
	(
		// 광원으로부터의 거리
		XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
		[&]()
		{

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			// 상수 버퍼 업데이트
			m_deviceContext->UpdateSubresource(m_worldNormalConstantBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);

			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);

			for (const auto& [model, material] : m_modelsAndMaterials)
			{
				// 재질 텍스처 셰이더에 설정
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::BaseColor), 1, material.baseColorTextureSRV.GetAddressOf());

				for (const Mesh& mesh : model->meshes)
				{
					resourceManager.SetPrimitiveTopology(mesh.topology);
					// 메쉬 버퍼 설정
					constexpr UINT STRIDE = sizeof(Vertex);
					constexpr UINT OFFSET = 0;
					m_deviceContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &STRIDE, &OFFSET);
					m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
					m_deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
				}
			}
		}
	);

	// 디버그 - 경계 상자 렌더링
	#ifdef _DEBUG
	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::max(),
		[&]()
		{
			// 프러스텀 컬링
			if (m_transformedbBoundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;

			ResourceManager& resourceManager = ResourceManager::GetInstance();

			m_deviceContext->IASetInputLayout(m_boundingBoxVertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_boundingBoxVertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_boundingBoxPixelShader.Get(), nullptr, 0);

			resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			// 경계 상자 그리기
			array<XMFLOAT3, 8> boxVertices = {};
			m_transformedbBoundingBox.GetCorners(boxVertices.data());

			LineBuffer lineBufferData = {};

			if (m_renderBoundingBox)
			{
				for (const auto& [startIndex, endIndex] : BOX_LINE_INDICES)
				{
					lineBufferData.linePoints[0] = XMFLOAT4{ boxVertices[startIndex].x, boxVertices[startIndex].y, boxVertices[startIndex].z, 1.0f };
					lineBufferData.linePoints[1] = XMFLOAT4{ boxVertices[endIndex].x, boxVertices[endIndex].y, boxVertices[endIndex].z, 1.0f };
					lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
					lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
					m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
					m_deviceContext->Draw(2, 0);
				}
			}

			if (m_renderSubMeshBoundingBoxes)
			{
				for (const auto& [model, material] : m_modelsAndMaterials)
				{
					for (const Mesh& mesh : model->meshes)
					{
						BoundingBox meshTransformedBox = {};
						mesh.boundingBox.Transform(meshTransformedBox, m_owner->GetWorldMatrix());
						meshTransformedBox.GetCorners(boxVertices.data());

						for (const auto& [startIndex, endIndex] : BOX_LINE_INDICES)
						{
							lineBufferData.linePoints[0] = XMFLOAT4{ boxVertices[startIndex].x, boxVertices[startIndex].y, boxVertices[startIndex].z, 1.0f };
							lineBufferData.linePoints[1] = XMFLOAT4{ boxVertices[endIndex].x, boxVertices[endIndex].y, boxVertices[endIndex].z, 1.0f };
							lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
							lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
							m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
							m_deviceContext->Draw(2, 0);
						}
					}
				}
			}
		}
	);
	#endif
}

#ifdef _DEBUG
void ModelComponent::RenderImGui()
{
	array<char, 256> vsShaderNameBuffer = {};
	strcpy_s(vsShaderNameBuffer.data(), vsShaderNameBuffer.size(), m_vsShaderName.c_str());
	if (ImGui::InputText("Vertex Shader Name", vsShaderNameBuffer.data(), sizeof(vsShaderNameBuffer))) m_vsShaderName = vsShaderNameBuffer.data();

	array<char, 256> psShaderNameBuffer = {};
	strcpy_s(psShaderNameBuffer.data(), psShaderNameBuffer.size(), m_psShaderName.c_str());
	if (ImGui::InputText("Pixel Shader Name", psShaderNameBuffer.data(), sizeof(psShaderNameBuffer))) m_psShaderName = psShaderNameBuffer.data();

	if(ImGui::Button("Load Shaders")) CreateShaders();
	ImGui::Separator();

	if (ImGui::TreeNode("Model and Materials"))
	{
		for (size_t i = 0; i < m_modelsAndMaterials.size(); ++i)
		{
			if (ImGui::TreeNode(reinterpret_cast<void*>(i), "Model and Material %zu", i))
			{
				// 모델 파일 이름
				array<char, 256> modelFileNameBuffer = {};
				strcpy_s(modelFileNameBuffer.data(), modelFileNameBuffer.size(), m_modelAndMaterialFileNames[i].first.c_str());
				if (ImGui::InputText("Model File Name", modelFileNameBuffer.data(), sizeof(modelFileNameBuffer))) m_modelAndMaterialFileNames[i].first = modelFileNameBuffer.data();

				// 재질 파일 이름
				array<char, 256> materialFileNameBuffer = {};
				strcpy_s(materialFileNameBuffer.data(), materialFileNameBuffer.size(), m_modelAndMaterialFileNames[i].second.c_str());
				if (ImGui::InputText("Material File Name", materialFileNameBuffer.data(), sizeof(materialFileNameBuffer))) m_modelAndMaterialFileNames[i].second = materialFileNameBuffer.data();

				if (ImGui::Button("Load Model and Material"))
				{
					ResourceManager& resourceManager = ResourceManager::GetInstance();
					m_modelsAndMaterials[i] = { resourceManager.LoadModel(m_modelAndMaterialFileNames[i].first), resourceManager.LoadMaterial(m_modelAndMaterialFileNames[i].second) };
					UpdateBoundingBox();
				}

				ImGui::SameLine();
				bool removeThis = ImGui::Button("Remove");

				// 텍스처 미리보기
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Material Textures");
				auto ShowTextureSlot = [](const char* name, ID3D11ShaderResourceView* srv)
					{
						ImGui::PushID(name);
						ImGui::Text("%s", name);
						if (srv)
						{
							ImGui::Image((ImTextureID)srv, ImVec2(64, 64), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f));
							if (ImGui::IsItemHovered())
							{
								ImGui::BeginTooltip();
								ImGui::Text("%s Preview", name);
								ImGui::Image((ImTextureID)srv, ImVec2(256, 256));
								ImGui::EndTooltip();
							}
						}
						else ImGui::Button("(Empty)", ImVec2(64, 64));
						ImGui::PopID();
					};

				Material& material = m_modelsAndMaterials[i].second;

				ImGui::Columns(4, "textureSlots");
				ShowTextureSlot("Base Color", material.baseColorTextureSRV.Get());
				ImGui::NextColumn();
				ShowTextureSlot("Normal", material.normalTextureSRV.Get());
				ImGui::NextColumn();
				ShowTextureSlot("ORM (Occl/Rough/Met)", material.ORMTextureSRV.Get());
				ImGui::NextColumn();
				ShowTextureSlot("Emission", material.emissionTextureSRV.Get());
				ImGui::Columns(1);

				// 재질 팩터
				ImGui::ColorEdit4("BaseColor Factor", &material.m_materialFactor.baseColorFactor.x);
				ImGui::DragFloat("Ambient Occlusion Factor", &material.m_materialFactor.ambientOcclusionFactor, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Roughness Factor", &material.m_materialFactor.roughnessFactor, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Metallic Factor", &material.m_materialFactor.metallicFactor, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Normal Scale", &material.m_materialFactor.normalScale, 0.01f, 0.0f, 1.0f);
				ImGui::ColorEdit3("Emission Factor", &material.m_materialFactor.emissionFactor.x);

				ImGui::TreePop();

				if (removeThis)
				{
					m_modelsAndMaterials.erase(m_modelsAndMaterials.begin() + i);
					m_modelAndMaterialFileNames.erase(m_modelAndMaterialFileNames.begin() + i);
					UpdateBoundingBox();
					break;
				}
			}
		}

		if (ImGui::Button("Add Model and Material"))
		{
			ResourceManager& resourceManager = ResourceManager::GetInstance();

			const pair<string, string> defaultName = { "box.fbx", "" };
			m_modelAndMaterialFileNames.emplace_back(defaultName);
			m_modelsAndMaterials.emplace_back(resourceManager.LoadModel(defaultName.first), resourceManager.LoadMaterial(defaultName.second));
		}

		ImGui::TreePop();
	}
	ImGui::Separator();

	// 디졸브
	if (ImGui::TreeNode("Dissolve Effect Settings"))
	{
		if (ImGui::CollapsingHeader("Dissolve Effect", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat("Threshold", &m_dissolveData.DissolveThreshold, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Edge Width", &m_dissolveData.DissolveEdgeWidth, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Edge Intensity", &m_dissolveData.DissolveEdgeIntensity, 0.1f, 0.0f, 100.0f);
			ImGui::DragFloat("Padding", &m_dissolveData.DissolvePadding, 0.01f, 0.0f, 1.0f); // 누락된 항목 추가
			ImGui::ColorEdit4("Edge Color", &m_dissolveData.DissolveEdgeColor.x, ImGuiColorEditFlags_HDR);
		}

		// 노이즈 선택
		const char noiseItems[] =
		{
			#define X(name) #name "\0"
			Noise_LIST
			#undef X
		};
		ImGui::Combo("Select Noise", &m_selectedNoiseIndex, noiseItems);
		com_ptr<ID3D11ShaderResourceView> ssrrvv1 = nullptr;
		ssrrvv1 = ResourceManager::GetInstance().GetNoise(m_selectedNoiseIndex);
		ImGui::Image
		(
			(ImTextureID)ssrrvv1.Get(),
			ImVec2(64, 64)
		);

		ImGui::TreePop();
	}

	ImGui::Separator();
	int blendStateInt = static_cast<int>(m_blendState);
	if (ImGui::Combo("Blend State", &blendStateInt, "Opaque\0AlphaToCoverage\0AlphaBlend\0")) m_blendState = static_cast<BlendState>(blendStateInt);
	int rasterStateInt = static_cast<int>(m_rasterState);
	if (ImGui::Combo("Raster State", &rasterStateInt, "BackBuffer\0Solid\0Wireframe\0")) m_rasterState = static_cast<RasterState>(rasterStateInt);

	ImGui::Separator();
	ImGui::Checkbox("Render Bounding Box", &m_renderBoundingBox);
	ImGui::Checkbox("Render SubMesh Bounding Boxes", &m_renderSubMeshBoundingBoxes);
}
#endif

void ModelComponent::Finalize()
{
	auto it = find(s_modelComponents.begin(), s_modelComponents.end(), this);
	if (it != s_modelComponents.end()) s_modelComponents.erase(it);
}

nlohmann::json ModelComponent::Serialize()
{
	nlohmann::json jsonData;

	jsonData["vsShaderName"] = m_vsShaderName;
	jsonData["psShaderName"] = m_psShaderName;

	jsonData["modelAndMaterialFileNames"] = nlohmann::json::array();
	for (const auto& [modelName, materialName] : m_modelAndMaterialFileNames)
	{
		nlohmann::json pairJson;
		pairJson["modelFileName"] = modelName;
		pairJson["materialFileName"] = materialName;
		jsonData["modelAndMaterialFileNames"].push_back(pairJson);
	}

	jsonData["blendState"] = static_cast<int>(m_blendState);
	jsonData["rasterState"] = static_cast<int>(m_rasterState);

	// 디졸브 이펙트
	jsonData["dissolveData"]["DissolveThreshold"] = m_dissolveData.DissolveThreshold;
	jsonData["dissolveData"]["DissolveEdgeWidth"] = m_dissolveData.DissolveEdgeWidth;
	jsonData["dissolveData"]["DissolveEdgeIntensity"] = m_dissolveData.DissolveEdgeIntensity;
	jsonData["dissolveData"]["DissolveEdgeColor"] = { m_dissolveData.DissolveEdgeColor.x, m_dissolveData.DissolveEdgeColor.y, m_dissolveData.DissolveEdgeColor.z, m_dissolveData.DissolveEdgeColor.w };

	//노이즈 뭐 쓸지
	jsonData["NoiseIndex"] = m_selectedNoiseIndex;

	return jsonData;
}

void ModelComponent::Deserialize(const nlohmann::json& jsonData)
{
	if (jsonData.contains("vsShaderName")) m_vsShaderName = jsonData["vsShaderName"].get<string>();
	if (jsonData.contains("psShaderName")) m_psShaderName = jsonData["psShaderName"].get<string>();

	if (jsonData.contains("modelAndMaterialFileNames"))
	{
		m_modelAndMaterialFileNames.clear();
		for (const auto& pairJson : jsonData["modelAndMaterialFileNames"])
		{
			string modelFileName = pairJson["modelFileName"].get<string>();
			string materialFileName = pairJson["materialFileName"].get<string>();
			m_modelAndMaterialFileNames.emplace_back(modelFileName, materialFileName);
		}
	}

	if (jsonData.contains("blendState")) m_blendState = static_cast<BlendState>(jsonData["blendState"].get<int>());
	if (jsonData.contains("rasterState")) m_rasterState = static_cast<RasterState>(jsonData["rasterState"].get<int>());

	if (jsonData.contains("dissolveData"))
	{
		if (jsonData["dissolveData"].contains("DissolveThreshold")) m_dissolveData.DissolveThreshold = jsonData["dissolveData"]["DissolveThreshold"].get<float>();
		if (jsonData["dissolveData"].contains("DissolveEdgeWidth")) m_dissolveData.DissolveEdgeWidth = jsonData["dissolveData"]["DissolveEdgeWidth"].get<float>();
		if (jsonData["dissolveData"].contains("DissolveEdgeIntensity")) m_dissolveData.DissolveEdgeIntensity = jsonData["dissolveData"]["DissolveEdgeIntensity"].get<float>();
		if (jsonData["dissolveData"].contains("DissolveEdgeColor"))
		{
			m_dissolveData.DissolveEdgeColor = XMFLOAT4
			(
				jsonData["dissolveData"]["DissolveEdgeColor"][0].get<float>(),
				jsonData["dissolveData"]["DissolveEdgeColor"][1].get<float>(),
				jsonData["dissolveData"]["DissolveEdgeColor"][2].get<float>(),
				jsonData["dissolveData"]["DissolveEdgeColor"][3].get<float>()
			);
		}
	}

	if (jsonData.contains("NoiseIndex")) m_selectedNoiseIndex = jsonData["NoiseIndex"].get<int>();
}

void ModelComponent::CreateShaders()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_vertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout(m_vsShaderName, m_inputElements);
	m_pixelShader = resourceManager.GetPixelShader(m_psShaderName);

	#ifdef _DEBUG
	m_boundingBoxVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_boundingBoxPixelShader = resourceManager.GetPixelShader("PSColor.hlsl");
	#endif
}

void ModelComponent::UpdateBoundingBox()
{
	m_boundingBox = {};
	for (const auto& [model, material] : m_modelsAndMaterials) BoundingBox::CreateMerged(m_boundingBox, m_boundingBox, model->boundingBox);
}
