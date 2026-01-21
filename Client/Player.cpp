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

REGISTER_TYPE(Player)

using namespace std;
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
	InputManager& input = InputManager::GetInstance();

	if (input.GetKey(KeyCode::W)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Forward);
	if (input.GetKey(KeyCode::S)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Backward);
	if (input.GetKey(KeyCode::A)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Left);
	if (input.GetKey(KeyCode::D)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Right);
	if (input.GetKey(KeyCode::Space)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Up);
	if (input.GetKey(KeyCode::Shift)) m_cameraObject->MoveDirection(deltaTime * 5.0f, Direction::Down);

	if (input.GetKeyDown(KeyCode::MouseLeft))
	{
		float distance = 0.0f;
		const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
		const XMVECTOR& origin = mainCamera.GetPosition();
		const XMVECTOR& direction = mainCamera.GetForwardVector();
		GameObjectBase* hit = ColliderComponent::CheckCollision(origin, direction, distance);
		if (hit)
		{
			if (dynamic_cast<Enemy*>(hit)) hit->SetAlive(false);

			LineBuffer lineBuffer = {};
			XMStoreFloat4(&lineBuffer.linePoints[0], m_gunObject->GetWorldPosition());
			lineBuffer.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
			XMStoreFloat4(&lineBuffer.linePoints[1], XMVectorAdd(origin, XMVectorScale(direction, distance)));
			lineBuffer.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };

			m_lineBuffers.emplace_back(lineBuffer, 0.5f);
		}
	}
	if (input.GetKeyDown(KeyCode::MouseRight))
	{
		vector<GameObjectBase*> hits = ColliderComponent::CheckCollision(CameraComponent::GetMainCamera().GetBoundingFrustum());

		for (size_t i = 0; i < hits.size(); ++i)
		{
			if (!dynamic_cast<Enemy*>(hits[i])) continue;

			hits[i]->SetAlive(false);
			LineBuffer lineBuffer = {};
			XMStoreFloat4(&lineBuffer.linePoints[0], m_gunObject->GetWorldPosition());
			lineBuffer.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
			XMStoreFloat4(&lineBuffer.linePoints[1], hits[i]->GetWorldPosition());
			lineBuffer.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };

			m_lineBuffers.emplace_back(lineBuffer, 0.5f);
		}

	}

	for_each(m_lineBuffers.begin(), m_lineBuffers.end(), [&](auto& pair) { pair.second -= deltaTime; });
	if (!m_lineBuffers.empty() && m_lineBuffers.front().second < 0.0f) m_lineBuffers.pop_front();
}

void Player::Render()
{
	if (m_lineBuffers.empty()) return;

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

			for (const auto& [lineBuffer, time] : m_lineBuffers)
			{
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBuffer, 0, 0);
				deviceContext->Draw(2, 0);
			}
		}
	);
}