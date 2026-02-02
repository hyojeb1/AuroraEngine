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
	m_modelFileName = "test5.fbx";

	m_inputElements.push_back(InputElement::Blendindex);  
	m_inputElements.push_back(InputElement::Blendweight); 
}

void SkinnedModelComponent::Initialize()
{
	ModelComponent::Initialize();

	ResourceManager& resourceManager = ResourceManager::GetInstance();

	m_boneConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Bone);

	if (m_model)
	{
		animator_ = make_shared<Animator>(m_model);
		if (!m_model->animations.empty())
		{
			animator_->PlayAnimation(m_model->animations.front().name);
			//animator_->PlayAnimation("rig|rigAction");
		}
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
	m_model->boundingBox.Transform(transformedBoundingBox, m_owner->GetWorldMatrix());
	XMVECTOR boxCenter = XMLoadFloat3(&transformedBoundingBox.Center);
	XMVECTOR boxExtents = XMLoadFloat3(&transformedBoundingBox.Extents);

	const XMVECTOR& sortPoint = renderer.GetRenderSortPoint();

	renderer.RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
		XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
		[&]()
		{
			if (!m_model) return;
			// 프러스텀 컬링 (필요시 추가)
			// if (transformedBoundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			m_deviceContext->UpdateSubresource(m_worldNormalConstantBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);
			m_deviceContext->UpdateSubresource(m_boneConstantBuffer.Get(), 0, nullptr, &m_boneBufferData, 0, 0);

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

				constexpr UINT stride = sizeof(Vertex);
				constexpr UINT offset = 0;
				m_deviceContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
				m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				m_deviceContext->UpdateSubresource(m_materialFactorConstantBuffer.Get(), 0, nullptr, &m_materialFactorData, 0, 0);

				m_deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
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
			if (m_boundingBox.Intersects(mainCamera.GetBoundingFrustum()) == false) return;

			ResourceManager& resourceManager = ResourceManager::GetInstance();

			m_deviceContext->IASetInputLayout(m_boundingBoxVertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_boundingBoxVertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_boundingBoxPixelShader.Get(), nullptr, 0);

			ResourceManager::GetInstance().SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

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
					lineBufferData.lineColors[0] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
					lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };
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
						lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
						lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
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
void SkinnedModelComponent::RenderImGui()
{
	ModelComponent::RenderImGui();

	if (!m_model || !animator_)		return;

	if (ImGui::CollapsingHeader("Animator Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		std::string currentAnimName = animator_->GetCurrentAnimationName();
		ImGui::Text("Current State: ");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "%s", currentAnimName.c_str());
		ImGui::Spacing();

		if (ImGui::BeginCombo("Play Animation", currentAnimName.c_str()))
		{
			for (const auto& clip : m_model->animations)
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