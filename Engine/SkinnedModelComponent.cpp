/// SkinnedModelComponent.cpp의 시작
#include "stdafx.h"
#include "SkinnedModelComponent.h"

#include "Renderer.h"
#include "ResourceManager.h"
#include "TimeManager.h"
#include "GameObjectBase.h"
#include "CameraComponent.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(SkinnedModelComponent)

namespace
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

} //End fo Scope : namespace



void SkinnedModelComponent::Initialize()
{
	m_deviceContext = Renderer::GetInstance().GetDeviceContext();
	m_worldNormalData = &m_owner->GetWorldNormalBuffer();

	ResourceManager& resourceManager = ResourceManager::GetInstance();

	CreateShaders();

	m_worldMatrixConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::WorldNormal);
	m_boneConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Bone);
	m_materialConstantBuffer = resourceManager.GetConstantBuffer(PSConstBuffers::MaterialFactor);
	m_lineConstantBuffer = resourceManager.GetConstantBuffer(VSConstBuffers::Line);
	m_model = resourceManager.LoadSkinnedModel(m_modelFileName);

}


void SkinnedModelComponent::Render()
{
	const XMMATRIX worldMatrix = m_owner->GetWorldMatrix();

	BoundingBox transformedBoundingBox = {};
	m_model->boundingBox.Transform(transformedBoundingBox, worldMatrix);
	XMVECTOR boxCenter = XMLoadFloat3(&transformedBoundingBox.Center);
	XMVECTOR boxExtents = XMLoadFloat3(&transformedBoundingBox.Extents);

	const XMVECTOR& sortPoint = Renderer::GetInstance().GetRenderSortPoint();

	Renderer::GetInstance().RENDER_FUNCTION(RenderStage::Scene, m_blendState).emplace_back
	(
		// 카메라로부터의 거리
		XMVectorGetX(XMVector3LengthSq(sortPoint - XMVectorClamp(sortPoint, boxCenter - boxExtents, boxCenter + boxExtents))),
		[&]()
		{
			if (!m_model) return;

			m_deviceContext->UpdateSubresource(m_worldMatrixConstantBuffer.Get(), 0, nullptr, m_worldNormalData, 0, 0);

			m_deviceContext->UpdateSubresource(m_boneConstantBuffer.Get(), 0, nullptr, &m_boneBufferData, 0, 0);

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetRasterState(m_rasterState);

			m_deviceContext->IASetInputLayout(m_vertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_vertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

			
			for (const SkinnedMesh& mesh : m_model->skinnedMeshes)
			{
				resourceManager.SetPrimitiveTopology(mesh.topology);

				// 메쉬 버퍼 설정
				constexpr UINT stride = sizeof(SkinnedVertex);
				constexpr UINT offset = 0;
				m_deviceContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
				m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				// 재질 팩터 설정
				m_deviceContext->UpdateSubresource(m_materialConstantBuffer.Get(), 0, nullptr, &m_materialFactorData, 0, 0);

				// 재질 텍스처 셰이더에 설정
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Albedo), 1, mesh.materialTexture.albedoTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::ORM), 1, mesh.materialTexture.ORMTextureSRV.GetAddressOf());
				m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlots::Normal), 1, mesh.materialTexture.normalTextureSRV.GetAddressOf());



				m_deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
			}
		}
	);
	Renderer::GetInstance().RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::max(),
		[&]()
		{
			if (!m_bRenderSkeletonLines || !m_model->skeleton.root) return;

			vector<XMFLOAT4> lineVertices = {};
			vector<XMFLOAT3> jointPositions = {};
			CollectSkeletonData(*m_model->skeleton.root, m_owner->GetWorldMatrix(), lineVertices, jointPositions);

			if (lineVertices.empty()) return;

			// 라인 셰이더 설정
			m_deviceContext->IASetInputLayout(m_lineVertexShaderAndInputLayout.second.Get());
			m_deviceContext->VSSetShader(m_lineVertexShaderAndInputLayout.first.Get(), nullptr, 0);
			m_deviceContext->PSSetShader(m_linePixelShader.Get(), nullptr, 0);

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			for (size_t i = 0; i < lineVertices.size(); i += 2)
			{
				LineBuffer lineBufferData = {};
				lineBufferData.linePoints[0] = XMVectorSet(lineVertices[i].x, lineVertices[i].y, lineVertices[i].z, 1.0f);
				lineBufferData.linePoints[1] = XMVectorSet(lineVertices[i + 1].x, lineVertices[i + 1].y, lineVertices[i + 1].z, 1.0f);
				lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f };

				m_deviceContext->UpdateSubresource(m_lineConstantBuffer.Get(), 0, nullptr, &lineBufferData, 0, 0);
				m_deviceContext->Draw(2, 0);
			}
		}
	);
}


void SkinnedModelComponent::RenderImGui()
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
		m_model = ResourceManager::GetInstance().LoadSkinnedModel(m_modelFileName);
		CreateShaders();
	}

	ImGui::Separator();
	// 재질 팩터
	ImGui::ColorEdit4("Albedo Factor", &m_materialFactorData.albedoFactor.x);

	ImGui::DragFloat("Ambient Occlusion Factor", &m_materialFactorData.ambientOcclusionFactor, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Roughness Factor", &m_materialFactorData.roughnessFactor, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Metallic Factor", &m_materialFactorData.metallicFactor, 0.01f, 0.0f, 1.0f);

	ImGui::DragFloat("IOR", &m_materialFactorData.ior, 0.01f, 1.0f, 3.0f);

	ImGui::DragFloat("Normal Scale", &m_materialFactorData.normalScale, 0.01f, 0.0f, 5.0f);
	ImGui::DragFloat("Height Scale", &m_materialFactorData.heightScale, 0.001f, 0.0f, 0.2f);

	ImGui::DragFloat("Light Factor", &m_materialFactorData.lightFactor, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Glow Factor", &m_materialFactorData.glowFactor, 0.01f, 0.0f, 1.0f);

	ImGui::ColorEdit4("Emission Factor", &m_materialFactorData.emissionFactor.x);

	ImGui::Separator();
	int blendStateInt = static_cast<int>(m_blendState);
	if (ImGui::Combo("Blend State", &blendStateInt, "Opaque\0AlphaToCoverage\0AlphaBlend\0")) m_blendState = static_cast<BlendState>(blendStateInt);
	int rasterStateInt = static_cast<int>(m_rasterState);
	if (ImGui::Combo("Raster State", &rasterStateInt, "BackBuffer\0Solid\0Wireframe\0")) m_rasterState = static_cast<RasterState>(rasterStateInt);

	ImGui::Separator();
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
	nlohmann::json jsonData;

	jsonData["vsShaderName"] = m_vsShaderName;
	jsonData["psShaderName"] = m_psShaderName;
	jsonData["modelFileName"] = m_modelFileName;

	// 재질 팩터
	jsonData["materialFactorData"]["albedoFactor"] = { m_materialFactorData.albedoFactor.x, m_materialFactorData.albedoFactor.y, m_materialFactorData.albedoFactor.z, m_materialFactorData.albedoFactor.w };
	
	jsonData["materialFactorData"]["ambientOcclusionFactor"] = m_materialFactorData.ambientOcclusionFactor;
	jsonData["materialFactorData"]["roughnessFactor"] = m_materialFactorData.roughnessFactor;
	jsonData["materialFactorData"]["metallicFactor"] = m_materialFactorData.metallicFactor;

	jsonData["materialFactorData"]["ior"] = m_materialFactorData.ior;

	jsonData["materialFactorData"]["normalScale"] = m_materialFactorData.normalScale;
	jsonData["materialFactorData"]["heightScale"] = m_materialFactorData.heightScale;

	jsonData["materialFactorData"]["lightFactor"] = m_materialFactorData.lightFactor;
	jsonData["materialFactorData"]["glowFactor"] = m_materialFactorData.glowFactor;

	jsonData["materialFactorData"]["emissionFactor"] = { m_materialFactorData.emissionFactor.x, m_materialFactorData.emissionFactor.y, m_materialFactorData.emissionFactor.z, m_materialFactorData.emissionFactor.w };

	jsonData["blendState"] = static_cast<int>(m_blendState);
	jsonData["rasterState"] = static_cast<int>(m_rasterState);

	return jsonData;
}

void SkinnedModelComponent::Deserialize(const nlohmann::json& jsonData)
{
	m_vsShaderName = jsonData["vsShaderName"].get<string>();
	m_psShaderName = jsonData["psShaderName"].get<string>();
	m_modelFileName = jsonData["modelFileName"].get<string>();

	// 재질 팩터
	m_materialFactorData.albedoFactor = XMFLOAT4
	(
		jsonData["materialFactorData"]["albedoFactor"][0].get<float>(),
		jsonData["materialFactorData"]["albedoFactor"][1].get<float>(),
		jsonData["materialFactorData"]["albedoFactor"][2].get<float>(),
		jsonData["materialFactorData"]["albedoFactor"][3].get<float>()
	);

	m_materialFactorData.ambientOcclusionFactor = jsonData["materialFactorData"]["ambientOcclusionFactor"].get<float>();
	m_materialFactorData.roughnessFactor = jsonData["materialFactorData"]["roughnessFactor"].get<float>();
	m_materialFactorData.metallicFactor = jsonData["materialFactorData"]["metallicFactor"].get<float>();

	m_materialFactorData.ior = jsonData["materialFactorData"]["ior"].get<float>();

	m_materialFactorData.normalScale = jsonData["materialFactorData"]["normalScale"].get<float>();
	m_materialFactorData.heightScale = jsonData["materialFactorData"]["heightScale"].get<float>();

	m_materialFactorData.lightFactor = jsonData["materialFactorData"]["lightFactor"].get<float>();
	m_materialFactorData.glowFactor = jsonData["materialFactorData"]["glowFactor"].get<float>();

	m_materialFactorData.emissionFactor = XMFLOAT4
	(
		jsonData["materialFactorData"]["emissionFactor"][0].get<float>(),
		jsonData["materialFactorData"]["emissionFactor"][1].get<float>(),
		jsonData["materialFactorData"]["emissionFactor"][2].get<float>(),
		jsonData["materialFactorData"]["emissionFactor"][3].get<float>()
	);

	m_blendState = static_cast<BlendState>(jsonData["blendState"].get<int>());
	m_rasterState = static_cast<RasterState>(jsonData["rasterState"].get<int>());
}

void SkinnedModelComponent::CreateShaders()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_vertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout(m_vsShaderName, m_inputElements);
	m_pixelShader = resourceManager.GetPixelShader(m_psShaderName);

	m_lineVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout(m_lineVSShaderName);
	m_linePixelShader = resourceManager.GetPixelShader(m_linePSShaderName);
}
/// SkinnedModelComponent.cpp의 끝