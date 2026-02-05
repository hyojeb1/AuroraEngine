#include "stdafx.h"
#include "SkinnedModelComponent.h"

#include "Renderer.h"
#include "ResourceManager.h"
#include "TimeManager.h"
#include "GameObjectBase.h"
#include "CameraComponent.h"

#include "Animator.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(SkinnedModelComponent)

SkinnedModelComponent::SkinnedModelComponent()
{
	m_vsShaderName = "VSModelSkinAnim.hlsl";
	m_modelAndMaterialFileNames.push_back({ "test5.fbx", "test5" });

	m_inputElements.push_back(InputElement::Blendindex);  
	m_inputElements.push_back(InputElement::Blendweight); 
}

void SkinnedModelComponent::Initialize()
{
	ModelComponent::Initialize();

	ResourceManager& resourceManager = ResourceManager::GetInstance();

	m_boneConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Bone);

	if (!m_modelsAndMaterials.empty())
	{
		const auto& [model, material] = m_modelsAndMaterials.front();
		animator_ = make_shared<Animator>(model);
		if (!model->animations.empty()) animator_->PlayAnimation(model->animations.front().name);
	}
}

void SkinnedModelComponent::Update()
{
	ModelComponent::Update();

	if (!animator_) return;

	const float& delta_time = TimeManager::GetInstance().GetDeltaTime();
	animator_->UpdateAnimation(delta_time);
}

void SkinnedModelComponent::Render()
{
	if (animator_)
	{
		const std::vector<DirectX::XMFLOAT4X4>& final_matrices = animator_->GetFinalBoneMatrices();
		const size_t bone_count = min(final_matrices.size(), static_cast<size_t>(MAX_BONES));
		for (size_t i = 0; i < bone_count; ++i)
		{
			XMMATRIX mat = XMLoadFloat4x4(&final_matrices[i]);
			mat = XMMatrixTranspose(mat);
			XMStoreFloat4x4(&m_boneBufferData.boneMatrix[i], mat);
		}
	}

	Renderer& renderer = Renderer::GetInstance();
	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

	BoundingBox transformedBoundingBox = {};
	XMVECTOR boxCenter = XMLoadFloat3(&transformedBoundingBox.Center);
	XMVECTOR boxExtents = XMLoadFloat3(&transformedBoundingBox.Extents);

	const XMVECTOR& sortPoint = renderer.GetRenderSortPoint();

	renderer.RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
		XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
		[&]()
		{
			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			m_deviceContext->UpdateSubresource(m_worldNormalConstantBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);
			m_deviceContext->UpdateSubresource(m_boneConstantBuffer.Get(), 0, nullptr, &m_boneBufferData, 0, 0);
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
					constexpr UINT stride = sizeof(Vertex);
					constexpr UINT offset = 0;

					m_deviceContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
					m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
					m_deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
				}
			}
		}
	);


	renderer.RENDER_FUNCTION(RenderStage::DirectionalLightShadow, m_blendState).emplace_back
	(
		XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
		[&]()
		{
			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(RasterState::Solid);

			m_deviceContext->UpdateSubresource(m_worldNormalConstantBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);
			m_deviceContext->UpdateSubresource(m_boneConstantBuffer.Get(), 0, nullptr, &m_boneBufferData, 0, 0);

			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);

			for (const auto& [model, material] : m_modelsAndMaterials)
			{
				for (const Mesh& mesh : model->meshes)
				{
					resourceManager.SetPrimitiveTopology(mesh.topology);
					constexpr UINT stride = sizeof(Vertex);
					constexpr UINT offset = 0;

					m_deviceContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
					m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
					m_deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
				}
			}
		}
	);

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

			ResourceManager::GetInstance().SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

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
					lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
					lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
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
							lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
							lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
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
void SkinnedModelComponent::RenderImGui()
{
	ModelComponent::RenderImGui();

	if (m_modelsAndMaterials.empty() || !animator_) return;

	if (ImGui::CollapsingHeader("Animator Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		std::string currentAnimName = animator_->GetCurrentAnimationName();
		ImGui::Text("Current State: ");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "%s", currentAnimName.c_str());
		ImGui::Spacing();

		if (ImGui::BeginCombo("Play Animation", currentAnimName.c_str()))
		{
			for (const auto& clip : m_modelsAndMaterials.front().first->animations)
			{
				bool isSelected = (currentAnimName == clip.name);
				if (ImGui::Selectable(clip.name.c_str(), isSelected))
				{
					animator_->PlayAnimation(clip.name, true, 0.5f);
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
}
#endif

void SkinnedModelComponent::Finalize()
{
	ModelComponent::Finalize();
}

nlohmann::json SkinnedModelComponent::Serialize()
{
	nlohmann::json jsonData = ModelComponent::Serialize();

	return jsonData;
}

void SkinnedModelComponent::Deserialize(const nlohmann::json& jsonData)
{
	ModelComponent::Deserialize(jsonData);
}

void SkinnedModelComponent::CreateShaders()
{
	ModelComponent::CreateShaders();
}