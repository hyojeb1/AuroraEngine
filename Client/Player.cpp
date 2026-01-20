#include "stdafx.h"
#include "Player.h"

#include "TimeManager.h"
#include "InputManager.h"
#include "ColliderComponent.h"
#include "CameraComponent.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "CamRotObject.h"
#include "ModelComponent.h"
#include "Enemy.h"

#include "GunObject.h"

REGISTER_TYPE(Player)

using namespace DirectX;

void Player::Initialize()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_lineVertexBufferAndShader = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_linePixelShader = resourceManager.GetPixelShader("PSColor.hlsl");

	m_cameraObject = GetChildGameObject("CamRotObject_2");
	m_gunObject = m_cameraObject->GetChildGameObject("Gun");
}

void Player::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();
	auto& input = InputManager::GetInstance();

	if (input.GetKey(KeyCode::W)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Forward);
	if (input.GetKey(KeyCode::S)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Backward);
	if (input.GetKey(KeyCode::A)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Left);
	if (input.GetKey(KeyCode::D)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Right);
	if (input.GetKey(KeyCode::Space)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Up);
	if (input.GetKey(KeyCode::Shift)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Down);

	if (input.GetKeyDown(KeyCode::MouseLeft))
	{
		float distance = 0.0f;

		if (auto gun = dynamic_cast<GunObject*>(m_gunObject))
		{
			gun->Fire();
		}

		const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
		const XMVECTOR& origin = mainCamera.GetPosition();
		const XMVECTOR& direction = mainCamera.GetForwardVector();
		GameObjectBase* hit = ColliderComponent::CheckCollision(origin, direction, distance);
		if (hit)
		{
			if (dynamic_cast<Enemy*>(hit)) hit->SetAlive(false);
			XMStoreFloat4(&m_lineBufferData.linePoints[0], m_gunObject->GetWorldPosition());
			m_lineBufferData.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
			XMStoreFloat4(&m_lineBufferData.linePoints[1], XMVectorAdd(origin, XMVectorScale(direction, distance)));
			m_lineBufferData.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
			m_lineDisplayTime = 0.5f;
		}
	}
	if (m_lineDisplayTime > 0.0f) m_lineDisplayTime -= deltaTime;
}

void Player::Render()
{
	if (m_lineDisplayTime < 0.0f) return;

	Renderer::GetInstance().RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		0.0f,
		[&]()
		{
			ResourceManager& resourceManager = ResourceManager::GetInstance();
			com_ptr<ID3D11DeviceContext> deviceContext = Renderer::GetInstance().GetDeviceContext();

			deviceContext->IASetInputLayout(m_lineVertexBufferAndShader.second.Get());
			deviceContext->VSSetShader(m_lineVertexBufferAndShader.first.Get(), nullptr, 0);
			deviceContext->PSSetShader(m_linePixelShader.Get(), nullptr, 0);

			resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &m_lineBufferData, 0, 0);
			deviceContext->Draw(2, 0);
		}
	);
}