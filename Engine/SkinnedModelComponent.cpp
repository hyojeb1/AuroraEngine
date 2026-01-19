/// SkinnedModelComponent.cpp의 시작
#include "stdafx.h"
#include "Animator.h"
#include "SkinnedModelComponent.h"

#include "Renderer.h"
#include "ResourceManager.h"
#include "TimeManager.h"
#include "GameObjectBase.h"
#include "CameraComponent.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(SkinnedModelComponent)

namespace HELPER_IN_SkinnedModelComponent_CPP
{
	void CollectSkeletonData(const SkeletonNode& node, const XMMATRIX& parentWorld, vector<XMFLOAT4>& lineVertices, vector<XMFLOAT3>& jointPositions)
	{
		const XMMATRIX localTransform = XMLoadFloat4x4(&node.localTransform);
		const XMMATRIX worldTransform = XMMatrixMultiply(localTransform, parentWorld);

		XMFLOAT3 worldPosition = {};
		XMStoreFloat3(&worldPosition, worldTransform.r[3]);
		jointPositions.push_back(worldPosition);

		for (const auto& child : node.children)
		{
			const XMMATRIX childLocalTransform = XMLoadFloat4x4(&child->localTransform);
			const XMMATRIX childWorldTransform = XMMatrixMultiply(childLocalTransform, worldTransform);

			XMFLOAT3 childWorldPosition = {};
			XMStoreFloat3(&childWorldPosition, childWorldTransform.r[3]);

			lineVertices.push_back({ XMFLOAT4(worldPosition.x, worldPosition.y, worldPosition.z, 1.0f) });
			lineVertices.push_back({ XMFLOAT4(childWorldPosition.x, childWorldPosition.y, childWorldPosition.z, 1.0f) });

			CollectSkeletonData(*child, worldTransform, lineVertices, jointPositions);
		}
	}

	void DrawSkeletonTreeImGui(const SkeletonNode& node)
	{
		const string label = node.boneIndex >= 0
			? node.name + " (bone " + to_string(node.boneIndex) + ")"
			: node.name;

		if (node.children.empty())
		{
			ImGui::BulletText("%s", label.c_str());
			return;
		}

		if (ImGui::TreeNode(label.c_str()))
		{
			for (const auto& child : node.children)
			{
				DrawSkeletonTreeImGui(*child);
			}
			ImGui::TreePop();
		}
	}

	// 벡터 출력을 위한 간단한 헬퍼
	void PrintVector(const char* label, XMVECTOR v)
	{
		XMFLOAT3 f;
		XMStoreFloat3(&f, v);
		std::cout << label << ": ["
			<< std::setw(8) << std::fixed << std::setprecision(3) << f.x << ", "
			<< std::setw(8) << std::fixed << std::setprecision(3) << f.y << ", "
			<< std::setw(8) << std::fixed << std::setprecision(3) << f.z << "]";
	}

	// 재귀적으로 스켈레톤 정보를 출력하는 함수
	void DebugPrintRecursive(const SkeletonNode* node, XMMATRIX parentWorld, int depth)
	{
		if (!node) return;

		XMMATRIX localTransform = XMLoadFloat4x4(&node->localTransform);

		// [중요] 행렬 곱 순서: Local * Parent
		XMMATRIX worldTransform = XMMatrixMultiply(localTransform, parentWorld);

		// 분해 (Decompose)
		XMVECTOR scale, rotQuat, trans;
		XMMatrixDecompose(&scale, &rotQuat, &trans, worldTransform);

		XMVECTOR localScale, localRot, localTrans;
		XMMatrixDecompose(&localScale, &localRot, &localTrans, localTransform);

		// 쿼터니언 -> 오일러 각 변환 (디버깅용, 라디안 -> 도)
		XMFLOAT4 q;
		XMStoreFloat4(&q, localRot);
		// 간단한 Pitch/Yaw/Roll 변환 (정확하지 않아도 회전 유무 확인 가능)
		float pitch = asinf(-2.0f * (q.y * q.z - q.w * q.x));
		float yaw = atan2f(2.0f * (q.x * q.z + q.w * q.y), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
		float roll = atan2f(2.0f * (q.x * q.y + q.w * q.z), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);

		// 라디안 -> 도
		auto ToDeg = [](float r) { return r * 180.0f / 3.141592f; };

		// 출력 포맷
		std::string indent(depth * 2, ' ');
		std::string prefix = indent + (depth > 0 ? "|- " : "");

		std::cout << prefix << "Node: " << node->name;
		if (node->boneIndex != -1) std::cout << " (Bone ID: " << node->boneIndex << ")";
		std::cout << std::endl;

		std::cout << indent << "   Local Pos: ["
			<< std::setw(6) << std::fixed << std::setprecision(3) << XMVectorGetX(localTrans) << ", "
			<< std::setw(6) << std::fixed << std::setprecision(3) << XMVectorGetY(localTrans) << ", "
			<< std::setw(6) << std::fixed << std::setprecision(3) << XMVectorGetZ(localTrans) << "]"
			<< " | Rot(Deg): ["
			<< std::setw(5) << ToDeg(pitch) << ", "
			<< std::setw(5) << ToDeg(yaw) << ", "
			<< std::setw(5) << ToDeg(roll) << "]"
			<< std::endl;

		std::cout << indent << "   World Pos: ["
			<< std::setw(6) << std::fixed << std::setprecision(3) << XMVectorGetX(trans) << ", "
			<< std::setw(6) << std::fixed << std::setprecision(3) << XMVectorGetY(trans) << ", "
			<< std::setw(6) << std::fixed << std::setprecision(3) << XMVectorGetZ(trans) << "]"
			<< std::endl;

		for (const auto& child : node->children)
		{
			DebugPrintRecursive(child.get(), worldTransform, depth + 1);
		}
	}

} //End fo Scope : namespace HELPER_IN_SkinnedModelComponent_CPP

using namespace HELPER_IN_SkinnedModelComponent_CPP;


SkinnedModelComponent::SkinnedModelComponent()
{
	// 스킨드 모델용 기본 셰이더로 변경
	m_vsShaderName = "VSModelSkinAnim.hlsl";
	m_modelFileName = "CastleGuard01_Walking.fbx";

	// 레이아웃 변경
	m_inputElements.push_back(InputElement::Blendindex);  // 1. Index 먼저 (Offset 60, UINT)
	m_inputElements.push_back(InputElement::Blendweight); // 2. Weight 나중 (Offset 76, FLOAT)
}

void SkinnedModelComponent::Initialize()
{
	// 부모 초기화 (셰이더 생성 및 모델 로드)
	ModelComponent::Initialize();

	ResourceManager& resourceManager = ResourceManager::GetInstance();

	// 애니메이션 관련 추가 초기화
	m_boneConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Bone);
	m_lineConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Line);

	// 부모가 로드한 m_model을 사용하여 Animator 생성
	if (m_model)
	{
		animator_ = make_shared<Animator>(m_model);
		if (!m_model->animations.empty())
		{
			animator_->PlayAnimation(m_model->animations.front().name);
		}
	}
}

void SkinnedModelComponent::Update()
{
	ModelComponent::Update();

	if (!animator_) return;

	const float delta_time = TimeManager::GetInstance().GetDeltaTime();
	animator_->UpdateAnimation(delta_time);
}

void SkinnedModelComponent::Render()
{
	if (animator_)
	{
		const auto& final_matrices = animator_->GetFinalBoneMatrices();
		const size_t bone_count = min(final_matrices.size(), static_cast<size_t>(MAX_BONES));
		for (size_t i = 0; i < bone_count; ++i)
		{
			//m_boneBufferData.boneMatrix[i] = final_matrices[i];
			m_boneBufferData.boneMatrix[i] = XMMatrixTranspose(final_matrices[i]);
		}
	}

	// [중요] 부모의 ModelComponent::Render()를 호출하지 않고,
	// 여기서 직접 렌더링 파이프라인을 설정해야 BoneBuffer를 바인딩할 수 있습니다.

	Renderer& renderer = Renderer::GetInstance();

	// 바운딩 박스 변환 (Update에서 이미 수행되었지만 안전을 위해 확인)
	BoundingBox transformedBoundingBox = {};
	m_model->boundingBox.Transform(transformedBoundingBox, m_owner->GetWorldMatrix());
	XMVECTOR boxCenter = XMLoadFloat3(&transformedBoundingBox.Center);
	XMVECTOR boxExtents = XMLoadFloat3(&transformedBoundingBox.Extents);

	const XMVECTOR& sortPoint = renderer.GetRenderSortPoint();

	// ==========================================================
	// 1. 일반 씬 렌더링 (Skinned Mesh 전용)
	// ==========================================================
	renderer.RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
		XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
		[&]()
		{
			if (!m_model) return;
			// 프러스텀 컬링 (필요시 추가)
			// if (transformedBoundingBox.Intersects(g_mainCamera->GetBoundingFrustum()) == false) return;

			// 1. 월드 행렬 업데이트
			m_deviceContext->UpdateSubresource(m_worldMatrixConstantBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);

			// 2. [핵심] 뼈 행렬 버퍼 업데이트 및 바인딩 (이게 없어서 터진 것임)
			m_deviceContext->UpdateSubresource(m_boneConstantBuffer.Get(), 0, nullptr, &m_boneBufferData, 0, 0);
			// VSSetConstantBuffers는 ResourceManager::Initialize에서 한번 설정되지만, 
			// 확실하게 하기 위해 리소스 매니저 설계를 확인해야 함. 
			// (현재 ResourceManager 구조상 Init때 Set하고 그 뒤로 UpdateSubresource만 하므로 바인딩은 유지됨.
			// 하지만 다른 객체가 슬롯을 덮어썼을 가능성에 대비하려면 여기서 바인딩해주는 게 안전함. 
			// 일단 기존 구조를 존중하여 Update만 수행)

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			// 3. 셰이더 설정 (VSModelSkinAnim)
			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

			for (const Mesh& mesh : m_model->meshes)
			{
				resourceManager.SetPrimitiveTopology(mesh.topology);

				constexpr UINT stride = sizeof(Vertex);
				constexpr UINT offset = 0;
				m_deviceContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
				m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				// 재질 팩터 업데이트
				m_deviceContext->UpdateSubresource(m_materialConstantBuffer.Get(), 0, nullptr, &m_materialFactorData, 0, 0);

				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Albedo), 1, mesh.materialTexture.albedoTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::ORM), 1, mesh.materialTexture.ORMTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Normal), 1, mesh.materialTexture.normalTextureSRV.GetAddressOf());

				m_deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
			}
		}
	);

	// ==========================================================
	// 2. 뼈대(Skeleton) 디버그 라인 그리기
	// ==========================================================
	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::max(),
		[&]()
		{
			if (!m_bRenderSkeletonLines || !m_model || !m_model->skeleton.root) return;

			vector<XMFLOAT4> lineVertices = {};
			vector<XMFLOAT3> jointPositions = {};
			CollectSkeletonData(*m_model->skeleton.root, m_owner->GetWorldMatrix(), lineVertices, jointPositions);

			if (lineVertices.empty()) return;

			m_deviceContext->IASetInputLayout(m_lineVertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_lineVertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_linePixelShader.Get(), nullptr, 0);

			ResourceManager::GetInstance().SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			for (size_t i = 0; i < lineVertices.size(); i += 2)
			{
				LineBuffer lineBufferData = {};
				lineBufferData.linePoints[0] = lineVertices[i];
				lineBufferData.linePoints[1] = lineVertices[i + 1];
				lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };

				m_deviceContext->UpdateSubresource(m_lineConstantBuffer.Get(), 0, nullptr, &lineBufferData, 0, 0);
				m_deviceContext->Draw(2, 0);
			}
		}
	);

	// ==========================================================
	// 3. 디버그 박스
	// ==========================================================

	#ifdef _DEBUG
	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::max(),
		[&]()
		{
			// 프러스텀 컬링
			if (m_boundingBox.Intersects(g_mainCamera->GetBoundingFrustum()) == false) return;

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

void SkinnedModelComponent::RenderImGui()
{
	ModelComponent::RenderImGui();

	// 기존 체크박스들...
	ImGui::Checkbox("Render Skeleton Lines", &m_bRenderSkeletonLines);
	ImGui::Checkbox("Show Skeleton Tree", &m_bShowSkeletonTree);
	ImGui::Checkbox("Show Joint Overlay", &m_bShowJointOverlay);

	// [추가된 디버그 버튼]
	if (ImGui::Button("Debug: Print Skeleton Info to Console"))
	{
		if (m_model && m_model->skeleton.root)
		{
			std::cout << "\n========== Skeleton Debug Info Start ==========\n";

			// 루트의 부모 행렬은 현재 컴포넌트(오브젝트)의 월드 행렬
			//XMMATRIX objectWorld = m_worldNormalData->worldMatrix;
			XMMATRIX objectWorld = m_owner->GetWorldMatrix();

			// 재귀 출력 시작
			DebugPrintRecursive(m_model->skeleton.root.get(), objectWorld, 0);

			std::cout << "========== Skeleton Debug Info End ============\n" << std::endl;
		}
		else
		{
			std::cout << "Error: Model or Skeleton Root is null!" << std::endl;
		}
	}

	// [기존 코드 아래에 추가]
	if (ImGui::Button("Debug: Print Animation Info"))
	{
		std::cout << "\n========== Animation Debug Info Start ==========\n";

		if (m_model)
		{
			size_t animCount = m_model->animations.size();
			std::cout << "Target Model: " << m_modelFileName << "\n";
			std::cout << "Total Animations Loaded: " << animCount << "\n\n";

			if (animCount > 0)
			{
				// 보기 좋게 표 형식으로 출력
				std::cout << std::left << std::setw(5) << "ID"
					<< std::setw(30) << "Clip Name"
					<< std::setw(15) << "Duration(Ticks)"
					<< std::setw(10) << "FPS" << "\n";
				std::cout << "------------------------------------------------------------\n";

				for (size_t i = 0; i < animCount; ++i)
				{
					const auto& clip = m_model->animations[i];
					std::cout << std::left << std::setw(5) << i
						<< std::setw(30) << clip.name
						<< std::setw(15) << std::fixed << std::setprecision(2) << clip.duration
						<< std::setw(10) << clip.ticks_per_second << "\n";
					std::cout << "  [Channel List] (" << clip.channels.size() << " channels)\n";
					for (const auto& pair : clip.channels)
					{
						// 채널 이름과 키프레임 개수 출력
						std::cout << "   - Name: " << pair.first
							<< " | Pos: " << pair.second.position_keys.size()
							<< " | Rot: " << pair.second.rotation_keys.size() << "\n";
					}
				}
				std::cout << "------------------------------------------------------------\n";
			}
			else
			{
				std::cout << "Warning: Model is loaded, but 'animations' vector is empty.\n";
			}
		}
		else
		{
			std::cout << "[Error] m_model is NULL. Please load a model first.\n";
		}



		std::cout << "========== Animation Debug Info End ============\n" << std::endl;
	}

	if (m_bShowSkeletonTree && m_model && m_model->skeleton.root)
	{
		ImGui::SeparatorText("Skeleton Hierarchy");
		DrawSkeletonTreeImGui(*m_model->skeleton.root);
	}

	if (m_bShowJointOverlay && m_model && m_model->skeleton.root && g_mainCamera)
	{
		vector<XMFLOAT4> lineVertices = {};
		vector<XMFLOAT3> jointPositions = {};
		CollectSkeletonData(*m_model->skeleton.root, m_owner->GetWorldMatrix(), lineVertices, jointPositions);

		const XMMATRIX viewProjection = XMMatrixMultiply(g_mainCamera->GetViewMatrix(), g_mainCamera->GetProjectionMatrix());
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

		for (const XMFLOAT3& jointPosition : jointPositions)
		{
			const XMVECTOR worldPos = XMVectorSet(jointPosition.x, jointPosition.y, jointPosition.z, 1.0f);
			const XMVECTOR clipPos = XMVector3TransformCoord(worldPos, viewProjection);

			XMFLOAT3 ndc = {};
			XMStoreFloat3(&ndc, clipPos);

			if (ndc.z < 0.0f || ndc.z > 1.0f) continue;
			if (ndc.x < -1.0f || ndc.x > 1.0f) continue;
			if (ndc.y < -1.0f || ndc.y > 1.0f) continue;

			const float screenX = (ndc.x * 0.5f + 0.5f) * displaySize.x;
			const float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * displaySize.y;

			drawList->AddCircleFilled(ImVec2(screenX, screenY), 3.0f, IM_COL32(255, 200, 0, 255));
		}
	}

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

	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_lineVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout(m_lineVSShaderName);
	m_linePixelShader = resourceManager.GetPixelShader(m_linePSShaderName);

}
/// SkinnedModelComponent.cpp의 끝