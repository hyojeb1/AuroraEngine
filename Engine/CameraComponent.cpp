#include "stdafx.h"
#include "CameraComponent.h"

#include "GameObjectBase.h"
#include "Renderer.h"

#ifdef _DEBUG
#include "ResourceManager.h"
#endif

REGISTER_TYPE(CameraComponent)

using namespace std;
using namespace DirectX;

CameraComponent* CameraComponent::s_mainCamera = nullptr;

const BoundingFrustum CameraComponent::GetBoundingFrustum() const
{	
	BoundingFrustum frustum = {};
	m_boundingFrustum.Transform(frustum, m_owner->GetWorldMatrix());

	return frustum;
}

XMFLOAT2 CameraComponent::WorldToScreenPosition(const XMVECTOR& worldPosition) const
{
	const DXGI_SWAP_CHAIN_DESC1& swapChainDesc = Renderer::GetInstance().GetSwapChainDesc();

	XMFLOAT2 result = {};

	XMStoreFloat2
	(
		&result,
		XMVector3Project
		(
			worldPosition,
			0.0f, 0.0f,
			static_cast<float>(swapChainDesc.Width), static_cast<float>(swapChainDesc.Height),
			0.0f, 1.0f,
			m_projectionMatrix,
			m_viewMatrix,
			XMMatrixIdentity()
		)
	);

	return result;
}

pair<XMVECTOR, XMVECTOR> CameraComponent::RayCast(float screenX, float screenY) const
{
	const DXGI_SWAP_CHAIN_DESC1& swapChainDesc = Renderer::GetInstance().GetSwapChainDesc();

	return
	{
		XMVector3Unproject
		(
			XMVectorSet(screenX, screenY, 0.0f, 1.0f),
			0.0f, 0.0f,
			static_cast<float>(swapChainDesc.Width), static_cast<float>(swapChainDesc.Height),
			0.0f, 1.0f,
			m_projectionMatrix,
			m_viewMatrix,
			XMMatrixIdentity()
		),
		XMVector3Unproject
		(
			XMVectorSet(screenX, screenY, 1.0f, 1.0f),
			0.0f, 0.0f,
			static_cast<float>(swapChainDesc.Width), static_cast<float>(swapChainDesc.Height),
			0.0f, 1.0f,
			m_projectionMatrix,
			m_viewMatrix,
			XMMatrixIdentity()
		)
	};
}

void CameraComponent::Initialize()
{
	#ifdef _DEBUG
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_boundingFrustumVertexShaderAndInputLayout = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_boundingFrustumPixelShader = resourceManager.GetPixelShader("PSColor.hlsl");
	#endif

	m_position = &m_owner->GetWorldMatrix().r[3];
	UpdateProjectionMatrix();
}

void CameraComponent::UpdateViewMatrix()
{
	m_forwardVector = m_owner->GetWorldDirectionVector(Direction::Forward);
	m_viewMatrix = XMMatrixLookAtLH
	(
		*m_position, // 카메라 위치
		XMVectorAdd(*m_position, m_forwardVector), // 카메라 앞 방향
		m_owner->GetWorldDirectionVector(Direction::Up) // 카메라 위 방향
	);
}

void CameraComponent::UpdateProjectionMatrix()
{
	m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fovY, Renderer::GetInstance().GetAspectRatio(), m_nearZ, m_farZ);
	m_boundingFrustum = BoundingFrustum(m_projectionMatrix);
}

void CameraComponent::Render()
{
	#ifdef _DEBUG
	Renderer& renderer = Renderer::GetInstance();
	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		numeric_limits<float>::max(),
		[&]()
		{
			BoundingFrustum frustum = GetBoundingFrustum();

			// 프러스텀 컬링
			if (frustum.Intersects(s_mainCamera->GetBoundingFrustum()) == false) return;

			ResourceManager& resourceManager = ResourceManager::GetInstance();
			com_ptr<ID3D11DeviceContext> deviceContext = renderer.GetDeviceContext();

			deviceContext->IASetInputLayout(m_boundingFrustumVertexShaderAndInputLayout.second.Get());
			deviceContext->VSSetShader(m_boundingFrustumVertexShaderAndInputLayout.first.Get(), nullptr, 0);
			deviceContext->PSSetShader(m_boundingFrustumPixelShader.Get(), nullptr, 0);

			ResourceManager::GetInstance().SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			// 경계 상자 그리기
			array<XMFLOAT3, 8> boxVertices = {};
			frustum.GetCorners(boxVertices.data());

			LineBuffer lineBufferData = {};

			for (const auto& [startIndex, endIndex] : BOX_LINE_INDICES)
			{
				lineBufferData.linePoints[0] = XMFLOAT4{ boxVertices[startIndex].x, boxVertices[startIndex].y, boxVertices[startIndex].z, 1.0f };
				lineBufferData.linePoints[1] = XMFLOAT4{ boxVertices[endIndex].x, boxVertices[endIndex].y, boxVertices[endIndex].z, 1.0f };
				lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
				lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBufferData, 0, 0);
				deviceContext->Draw(2, 0);
			}
		}
	);
	#endif
}

void CameraComponent::RenderImGui()
{
	float fovYInDegrees = XMConvertToDegrees(m_fovY);
	if (ImGui::DragFloat("FovY", &fovYInDegrees, 0.1f, 1.0f, 179.0f)) m_fovY = XMConvertToRadians(fovYInDegrees);
	ImGui::DragFloat("NearZ", &m_nearZ, 0.01f, 0.01f, m_farZ - 0.01f);
	ImGui::DragFloat("FarZ", &m_farZ, 1.0f, m_nearZ + 0.01f, 10000.0f);
}

void CameraComponent::Finalize()
{
	if (s_mainCamera == this) s_mainCamera = nullptr;
}

nlohmann::json CameraComponent::Serialize()
{
	nlohmann::json jsonData;

	jsonData["fovY"] = m_fovY;
	jsonData["nearZ"] = m_nearZ;
	jsonData["farZ"] = m_farZ;

	return jsonData;
}

void CameraComponent::Deserialize(const nlohmann::json& jsonData)
{
	if (jsonData.contains("fovY")) m_fovY = jsonData["fovY"].get<float>();
	if (jsonData.contains("nearZ")) m_nearZ = jsonData["nearZ"].get<float>();
	if (jsonData.contains("farZ")) m_farZ = jsonData["farZ"].get<float>();
}