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
		if (modelComp->m_boundingBox.Intersects(origin, dirNormalized, dist))
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

	m_model = resourceManager.LoadModel(m_modelFileName);
	m_worldNormalConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::WorldNormal);
	m_materialFactorConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::MaterialFactor);

	s_modelComponents.emplace_back(this);
}

void ModelComponent::Render()
{
	Renderer& renderer = Renderer::GetInstance();
	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

	m_model->boundingBox.Transform(m_boundingBox, m_owner->GetWorldMatrix());
	XMVECTOR boxCenter = XMLoadFloat3(&m_boundingBox.Center);
	XMVECTOR boxExtents = XMLoadFloat3(&m_boundingBox.Extents);

	const XMVECTOR& sortPoint = renderer.GetRenderSortPoint();

	// 일반 렌더링
	renderer.RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
		// 카메라로부터의 거리
		XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
		[&]()
		{
			// 프러스텀 컬링
			if (m_boundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			// 상수 버퍼 업데이트
			m_deviceContext->UpdateSubresource(m_worldNormalConstantBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);

			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

			// 재질 텍스처 셰이더에 설정
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::BaseColor), 1, m_model->materialTexture.baseColorTextureSRV.GetAddressOf());
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::ORM), 1, m_model->materialTexture.ORMTextureSRV.GetAddressOf());
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Normal), 1, m_model->materialTexture.normalTextureSRV.GetAddressOf());
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Emission), 1, m_model->materialTexture.emissionTextureSRV.GetAddressOf());
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Noise), 1, resourceManager.GetNoise(m_selectedNoiseIndex).GetAddressOf());

			for (const Mesh& mesh : m_model->meshes)
			{
				resourceManager.SetPrimitiveTopology(mesh.topology);

				// 메쉬 버퍼 설정
				constexpr UINT STRIDE = sizeof(Vertex);
				constexpr UINT OFFSET = 0;
				m_deviceContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &STRIDE, &OFFSET);
				m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				// 재질 팩터 설정
				m_deviceContext->UpdateSubresource(m_materialFactorConstantBuffer.Get(), 0, nullptr, &m_materialFactorData, 0, 0);

				m_deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
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

			// 재질 텍스처 셰이더에 설정
			m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::BaseColor), 1, m_model->materialTexture.baseColorTextureSRV.GetAddressOf());

			for (const Mesh& mesh : m_model->meshes)
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
	);

	// 디버그 - 경계 상자 렌더링
	#ifdef _DEBUG
	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::max(),
		[&]()
		{
			// 프러스텀 컬링
			if (m_boundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;

			ResourceManager& resourceManager = ResourceManager::GetInstance();

			m_deviceContext->IASetInputLayout(m_boundingBoxVertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_boundingBoxVertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_boundingBoxPixelShader.Get(), nullptr, 0);

			resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			// 경계 상자 그리기
			array<XMFLOAT3, 8> boxVertices = {};
			m_boundingBox.GetCorners(boxVertices.data());

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
				for (const Mesh& mesh : m_model->meshes)
				{
					BoundingBox meshTransformedBox = {};
					mesh.boundingBox.Transform(meshTransformedBox, m_owner->GetWorldMatrix());
					array<XMFLOAT3, 8> meshBoxVertices = {};
					meshTransformedBox.GetCorners(meshBoxVertices.data());

					for (const auto& [startIndex, endIndex] : BOX_LINE_INDICES)
					{
						lineBufferData.linePoints[0] = XMFLOAT4{ meshBoxVertices[startIndex].x, meshBoxVertices[startIndex].y, meshBoxVertices[startIndex].z, 1.0f };
						lineBufferData.linePoints[1] = XMFLOAT4{ meshBoxVertices[endIndex].x, meshBoxVertices[endIndex].y, meshBoxVertices[endIndex].z, 1.0f };
						lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
						lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
						m_deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
						m_deviceContext->Draw(2, 0);
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
	array<char, 256> modelFileNameBuffer = {};
	strcpy_s(modelFileNameBuffer.data(), modelFileNameBuffer.size(), m_modelFileName.c_str());
	if (ImGui::InputText("Model File Name", modelFileNameBuffer.data(), sizeof(modelFileNameBuffer))) m_modelFileName = modelFileNameBuffer.data();

	array<char, 256> vsShaderNameBuffer = {};
	strcpy_s(vsShaderNameBuffer.data(), vsShaderNameBuffer.size(), m_vsShaderName.c_str());
	if (ImGui::InputText("Vertex Shader Name", vsShaderNameBuffer.data(), sizeof(vsShaderNameBuffer))) m_vsShaderName = vsShaderNameBuffer.data();

	array<char, 256> psShaderNameBuffer = {};
	strcpy_s(psShaderNameBuffer.data(), psShaderNameBuffer.size(), m_psShaderName.c_str());
	if (ImGui::InputText("Pixel Shader Name", psShaderNameBuffer.data(), sizeof(psShaderNameBuffer))) m_psShaderName = psShaderNameBuffer.data();

	if (ImGui::Button("Load"))
	{
		m_model = ResourceManager::GetInstance().LoadModel(m_modelFileName);
		CreateShaders();
	}

	ImGui::Separator();
	// 재질 팩터
	ImGui::ColorEdit4("BaseColor Factor", &m_materialFactorData.baseColorFactor.x);

	ImGui::DragFloat("Ambient Occlusion Factor", &m_materialFactorData.ambientOcclusionFactor, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Roughness Factor", &m_materialFactorData.roughnessFactor, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Metallic Factor", &m_materialFactorData.metallicFactor, 0.01f, 0.0f, 10.0f);

	ImGui::DragFloat("Normal Scale", &m_materialFactorData.normalScale, 0.01f, 0.0f, 10.0f);

	ImGui::ColorEdit3("Emission Factor", &m_materialFactorData.emissionFactor.x);
	
	
	if (ImGui::CollapsingHeader("Dissolve Effect", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Threshold", &m_materialFactorData.DissolveThreshold, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Edge Width", &m_materialFactorData.DissolveEdgeWidth, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Edge Intensity", &m_materialFactorData.DissolveEdgeIntensity, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Padding", &m_materialFactorData.DissolvePadding, 0.01f, 0.0f, 1.0f); // 누락된 항목 추가
		ImGui::ColorEdit4("Edge Color", &m_materialFactorData.DissolveEdgeColor.x, ImGuiColorEditFlags_HDR);
	}


	ImGui::Separator();
	int blendStateInt = static_cast<int>(m_blendState);
	if (ImGui::Combo("Blend State", &blendStateInt, "Opaque\0AlphaToCoverage\0AlphaBlend\0")) m_blendState = static_cast<BlendState>(blendStateInt);
	int rasterStateInt = static_cast<int>(m_rasterState);
	if (ImGui::Combo("Raster State", &rasterStateInt, "BackBuffer\0Solid\0Wireframe\0")) m_rasterState = static_cast<RasterState>(rasterStateInt);

	ImGui::Separator();
	ImGui::Checkbox("Render Bounding Box", &m_renderBoundingBox);
	ImGui::Checkbox("Render SubMesh Bounding Boxes", &m_renderSubMeshBoundingBoxes);


	// 텍스처 미리보기 섹션
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Material Textures");

	if (m_model)
	{
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
				else
				{
					ImGui::Button("(Empty)", ImVec2(64, 64));
				}
				ImGui::PopID();
			};

		// 한 줄에 2개씩 보여주기 위해 Columns 사용 
		ImGui::Columns(2, "TextureColumns", false);

		ShowTextureSlot("Base Color", m_model->materialTexture.baseColorTextureSRV.Get());
		ImGui::NextColumn();

		ShowTextureSlot("Normal", m_model->materialTexture.normalTextureSRV.Get());
		ImGui::NextColumn();

		ShowTextureSlot("ORM (Occl/Rough/Met)", m_model->materialTexture.ORMTextureSRV.Get());
		ImGui::NextColumn();

		ShowTextureSlot("Emission", m_model->materialTexture.emissionTextureSRV.Get());

		ImGui::Columns(1);

		const char* noiseItems[] = {
		#define X(name) #name,
		Noise_LIST
		#undef X
		};

		ImGui::Combo("Select noise", &m_selectedNoiseIndex, noiseItems, NoiseData::COUNT);

		com_ptr<ID3D11ShaderResourceView> ssrrvv1 = nullptr;
		ssrrvv1 = ResourceManager::GetInstance().GetNoise(m_selectedNoiseIndex);
		ImGui::Image
		(
			(ImTextureID)ssrrvv1.Get(),
			ImVec2(64, 64)
		);

	}
	else
	{
		ImGui::TextDisabled("Model is not loaded.");
	}

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
	jsonData["modelFileName"] = m_modelFileName;

	// 재질 팩터
	jsonData["materialFactorData"]["baseColorFactor"] = { m_materialFactorData.baseColorFactor.x, m_materialFactorData.baseColorFactor.y, m_materialFactorData.baseColorFactor.z, m_materialFactorData.baseColorFactor.w };
	
	jsonData["materialFactorData"]["ambientOcclusionFactor"] = m_materialFactorData.ambientOcclusionFactor;
	jsonData["materialFactorData"]["roughnessFactor"] = m_materialFactorData.roughnessFactor;
	jsonData["materialFactorData"]["metallicFactor"] = m_materialFactorData.metallicFactor;

	jsonData["materialFactorData"]["normalScale"] = m_materialFactorData.normalScale;

	jsonData["materialFactorData"]["emissionFactor"] = { m_materialFactorData.emissionFactor.x, m_materialFactorData.emissionFactor.y, m_materialFactorData.emissionFactor.z, m_materialFactorData.emissionFactor.w };


	jsonData["materialFactorData"]["DissolveThreshold"] = m_materialFactorData.DissolveThreshold;
	jsonData["materialFactorData"]["DissolveEdgeWidth"] = m_materialFactorData.DissolveEdgeWidth;
	jsonData["materialFactorData"]["DissolveEdgeIntensity"] = m_materialFactorData.DissolveEdgeIntensity;
	jsonData["materialFactorData"]["DissolvePadding"] = m_materialFactorData.DissolvePadding;
	jsonData["materialFactorData"]["DissolveEdgeColor"] = { m_materialFactorData.DissolveEdgeColor.x, m_materialFactorData.DissolveEdgeColor.y, m_materialFactorData.DissolveEdgeColor.z, m_materialFactorData.DissolveEdgeColor.w };

	jsonData["blendState"] = static_cast<int>(m_blendState);
	jsonData["rasterState"] = static_cast<int>(m_rasterState);

	//노이즈 뭐 쓸지
	jsonData["NoiseIndex"] = m_selectedNoiseIndex;

	return jsonData;
}

void ModelComponent::Deserialize(const nlohmann::json& jsonData)
{
	if (jsonData.contains("vsShaderName")) m_vsShaderName = jsonData["vsShaderName"].get<string>();
	if (jsonData.contains("psShaderName")) m_psShaderName = jsonData["psShaderName"].get<string>();
	if (jsonData.contains("modelFileName")) m_modelFileName = jsonData["modelFileName"].get<string>();

	// 재질 팩터
	if (jsonData.contains("materialFactorData"))
	{
		m_materialFactorData.baseColorFactor = XMFLOAT4
		(
			jsonData["materialFactorData"]["baseColorFactor"][0].get<float>(),
			jsonData["materialFactorData"]["baseColorFactor"][1].get<float>(),
			jsonData["materialFactorData"]["baseColorFactor"][2].get<float>(),
			jsonData["materialFactorData"]["baseColorFactor"][3].get<float>()
		);
	}

	if (jsonData["materialFactorData"].contains("ambientOcclusionFactor")) m_materialFactorData.ambientOcclusionFactor = jsonData["materialFactorData"]["ambientOcclusionFactor"].get<float>();
	if (jsonData["materialFactorData"].contains("roughnessFactor")) m_materialFactorData.roughnessFactor = jsonData["materialFactorData"]["roughnessFactor"].get<float>();
	if (jsonData["materialFactorData"].contains("metallicFactor")) m_materialFactorData.metallicFactor = jsonData["materialFactorData"]["metallicFactor"].get<float>();
	if (jsonData["materialFactorData"].contains("normalScale")) m_materialFactorData.normalScale = jsonData["materialFactorData"]["normalScale"].get<float>();

	if (jsonData["materialFactorData"].contains("emissionFactor"))
	{
		m_materialFactorData.emissionFactor = XMFLOAT4
		(
			jsonData["materialFactorData"]["emissionFactor"][0].get<float>(),
			jsonData["materialFactorData"]["emissionFactor"][1].get<float>(),
			jsonData["materialFactorData"]["emissionFactor"][2].get<float>(),
			jsonData["materialFactorData"]["emissionFactor"][3].get<float>()
		);
	}

	if (jsonData["materialFactorData"].contains("DissolveThreshold")) m_materialFactorData.DissolveThreshold = jsonData["materialFactorData"]["DissolveThreshold"].get<float>();
	if (jsonData["materialFactorData"].contains("DissolveEdgeWidth")) m_materialFactorData.DissolveEdgeWidth = jsonData["materialFactorData"]["DissolveEdgeWidth"].get<float>();
	if (jsonData["materialFactorData"].contains("DissolveEdgeIntensity")) m_materialFactorData.DissolveEdgeIntensity = jsonData["materialFactorData"]["DissolveEdgeIntensity"].get<float>();
	if (jsonData["materialFactorData"].contains("DissolvePadding")) m_materialFactorData.DissolvePadding = jsonData["materialFactorData"]["DissolvePadding"].get<float>();

	if (jsonData["materialFactorData"].contains("DissolveEdgeColor"))
	{
		m_materialFactorData.emissionFactor = XMFLOAT4
		(
			jsonData["materialFactorData"]["DissolveEdgeColor"][0].get<float>(),
			jsonData["materialFactorData"]["DissolveEdgeColor"][1].get<float>(),
			jsonData["materialFactorData"]["DissolveEdgeColor"][2].get<float>(),
			jsonData["materialFactorData"]["DissolveEdgeColor"][3].get<float>()
		);
	}

	if (jsonData.contains("blendState")) m_blendState = static_cast<BlendState>(jsonData["blendState"].get<int>());
	if (jsonData.contains("rasterState")) m_rasterState = static_cast<RasterState>(jsonData["rasterState"].get<int>());

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