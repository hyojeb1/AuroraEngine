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

#include "FSMComponentGun.h"

REGISTER_TYPE(Player)

using namespace std;
using namespace DirectX;

void Player::Initialize()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_lineVertexBufferAndShader = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_linePixelShader = resourceManager.GetPixelShader("PSColor.hlsl");
	m_crosshairTextureAndOffset = resourceManager.GetTextureAndOffset("Crosshair.png");

	m_cameraObject = dynamic_cast<CamRotObject*>(GetChildGameObject("CamRotObject_2"));
	m_gunObject = m_cameraObject->GetChildGameObject("Gun");
	m_gunFSM = m_gunObject->CreateComponent<FSMComponentGun>();
}

void Player::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();
	InputManager& input = InputManager::GetInstance();

	PlayerMove(deltaTime, input);

	if (input.GetKeyDown(KeyCode::MouseLeft)) PlayerShoot();
	if (!m_isDeadEyeActive && input.GetKeyDown(KeyCode::MouseRight)) PlayerDeadEyeStart();
	if (m_isDeadEyeActive) PlayerDeadEye(deltaTime);

	for_each(m_lineBuffers.begin(), m_lineBuffers.end(), [&](auto& pair) { pair.second -= deltaTime; });
	if (!m_lineBuffers.empty() && m_lineBuffers.front().second < 0.0f) m_lineBuffers.pop_front();
	for_each(m_UINode.begin(), m_UINode.end(), [&](auto& time) { time -= deltaTime; });
	if (!m_UINode.empty() && m_UINode.front() < 0.0f) m_UINode.pop_front();
}

void Player::Render()
{
	Renderer& renderer = Renderer::GetInstance();

	RenderCrosshairUI(renderer);
	if (!m_lineBuffers.empty()) RenderLineBuffers(renderer);
	if (!m_deadEyeTargets.empty()) RenderDeadEyeTargetsUI(renderer);
	if (!m_UINode.empty()) RenderUINode(renderer);
}

void Player::Finalize()
{
	TimeManager::GetInstance().SetTimeScale(1.0f); // 시간 느린 상태에서 종료되는 상황 방지
}

void Player::PlayerMove(float deltaTime, InputManager& input)
{
	float yaw = XMVectorGetY(GetRotation()) + static_cast<float>(input.GetMouseDelta().x) * m_xSensitivity;
	SetRotation(XMVectorSet(0.0f, yaw, 0.0f, 0.0f));

	float forwardInput = 0.0f;
	float rightInput = 0.0f;
	if (input.GetKey(KeyCode::W)) forwardInput += m_moveSpeed;
	if (input.GetKey(KeyCode::S)) forwardInput -= m_moveSpeed;
	if (input.GetKey(KeyCode::A)) rightInput -= m_moveSpeed;
	if (input.GetKey(KeyCode::D)) rightInput += m_moveSpeed;

	// 대각선 이동 보정
	if (forwardInput != 0.0f && rightInput != 0.0f) { forwardInput *= 0.7071f; rightInput *= 0.7071f; }
	MovePosition(GetWorldDirectionVector(Direction::Forward) * forwardInput * deltaTime + GetWorldDirectionVector(Direction::Right) * rightInput * deltaTime);
}

void Player::PlayerShoot()
{
	if (!m_gunObject) return;

	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
	const XMVECTOR& origin = mainCamera.GetPosition();
	const XMVECTOR& direction = mainCamera.GetForwardVector();
	float distance = 0.0f;
	GameObjectBase* hit = ColliderComponent::CheckCollision(origin, direction, distance);
	if (hit)
	{
		if (m_gunFSM) m_gunFSM->Fire();
		if (Enemy* enemy = dynamic_cast<Enemy*>(hit)) enemy->Die();

		LineBuffer lineBuffer = {};
		XMStoreFloat4(&lineBuffer.linePoints[0], m_gunObject->GetWorldPosition());
		lineBuffer.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
		XMStoreFloat4(&lineBuffer.linePoints[1], XMVectorAdd(origin, XMVectorScale(direction, distance)));
		lineBuffer.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };

		m_lineBuffers.emplace_back(lineBuffer, 0.5f);
	}

	m_UINode.push_back(1.28f);
}

void Player::PlayerDeadEyeStart()
{
	const DXGI_SWAP_CHAIN_DESC1& swapChainDesc = Renderer::GetInstance().GetSwapChainDesc();
	float halfWidth = static_cast<float>(swapChainDesc.Width) * 0.5f;
	float halfHeight = static_cast<float>(swapChainDesc.Height) * 0.5f;

	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
	vector<GameObjectBase*> hits = ColliderComponent::CheckCollision(mainCamera.GetBoundingFrustum());

	bool hasEnemy = false;

	for (GameObjectBase* hit : hits)
	{
		if (Enemy* enemy = dynamic_cast<Enemy*>(hit))
		{
			hasEnemy = true;
			XMFLOAT2 distancePair = mainCamera.WorldToScreenPosition(enemy->GetWorldPosition());
			m_deadEyeTargets.emplace_back(powf(distancePair.x - halfWidth, 2) + powf(distancePair.y - halfHeight, 2), enemy);
		}
	}
	if (hasEnemy)
	{
		m_isDeadEyeActive = true;
		m_deadEyeTime = m_deadEyeDuration;

		TimeManager::GetInstance().SetTimeScale(0.1f); // 데드 아이 타임 활성화 시 시간 느리게
		m_cameraYSensitivity = m_cameraObject->GetSensitivity();
		m_cameraObject->SetSensitivity(0.01f); // 데드 아이 타임 활성화 시 카메라 감도 감소
		m_xSensitivity = m_cameraObject->GetSensitivity();

		Renderer::GetInstance().SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::Grayscale, true);

		sort(m_deadEyeTargets.begin(), m_deadEyeTargets.end(), [](const auto& a, const auto& b) { return get<0>(a) < get<0>(b); });
		if (m_deadEyeTargets.size() > 6) m_deadEyeTargets.resize(6);
	}
}

void Player::PlayerDeadEye(float deltaTime)
{
	Renderer::GetInstance().SetGrayScaleIntensity((1.0f - (m_deadEyeTime / m_deadEyeDuration)) * 2.0f);
	m_deadEyeTime -= deltaTime;

	if (m_deadEyeTime <= 0.0f) // 데드 아이 타임 종료
	{
		for (const auto& [timing, enemy] : m_deadEyeTargets)
		{
			if (timing > 999990.1f) continue; // 0.1초 이상 타이밍이 안맞으면 무시

			enemy->Die();

			LineBuffer lineBuffer = {};
			if (m_gunObject) XMStoreFloat4(&lineBuffer.linePoints[0], m_gunObject->GetWorldPosition());
			lineBuffer.lineColors[0] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
			XMStoreFloat4(&lineBuffer.linePoints[1], enemy->GetWorldPosition());
			lineBuffer.lineColors[1] = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };

			m_lineBuffers.emplace_back(lineBuffer, 0.5f);
		}
		PlayerDeadEyeEnd();
	}
}

void Player::PlayerDeadEyeEnd()
{
	if (m_gunObject) if (m_gunFSM) m_gunFSM->Fire();

	m_isDeadEyeActive = false;

	TimeManager::GetInstance().SetTimeScale(1.0f); // 시간 정상화
	m_cameraObject->SetSensitivity(m_cameraYSensitivity); // 카메라 감도 원래대로
	m_xSensitivity = m_cameraObject->GetSensitivity();

	m_deadEyeTargets.clear();

	Renderer& renderer = Renderer::GetInstance();
	renderer.SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::Grayscale, false);
	renderer.SetGrayScaleIntensity(0.0f);
}

void Player::RenderCrosshairUI(Renderer& renderer)
{
	renderer.UI_RENDER_FUNCTIONS().emplace_back([&]() { Renderer::GetInstance().RenderImageUIPosition(m_crosshairTextureAndOffset.first, { 0.5f, 0.5f }, m_crosshairTextureAndOffset.second); });
}

void Player::RenderLineBuffers(Renderer& renderer)
{
	renderer.RENDER_FUNCTION(RenderStage::Scene, BlendState::Opaque).emplace_back
	(
		0.0f,
		[&]()
		{
			ResourceManager& resourceManager = ResourceManager::GetInstance();
			com_ptr<ID3D11DeviceContext> deviceContext = Renderer::GetInstance().GetDeviceContext();

			deviceContext->IASetInputLayout(m_lineVertexBufferAndShader.second.Get());
			deviceContext->VSSetShader(m_lineVertexBufferAndShader.first.Get(), nullptr, 0);
			deviceContext->PSSetShader(m_linePixelShader.Get(), nullptr, 0);

			resourceManager.SetRasterState(RasterState::SolidCullNone);
			resourceManager.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			for (const auto& [lineBuffer, time] : m_lineBuffers)
			{
				deviceContext->UpdateSubresource(resourceManager.GetConstantBuffer(VSConstBuffers::Line).Get(), 0, nullptr, &lineBuffer, 0, 0);
				deviceContext->Draw(2, 0);
			}
		}
	);
}

void Player::RenderDeadEyeTargetsUI(Renderer& renderer)
{
	renderer.UI_RENDER_FUNCTIONS().emplace_back
	(
		[&]()
		{
			const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

			for (const auto& [timing, enemy] : m_deadEyeTargets)
			{
				Renderer::GetInstance().RenderImageScreenPosition(m_crosshairTextureAndOffset.first, mainCamera.WorldToScreenPosition(enemy->GetWorldPosition()), m_crosshairTextureAndOffset.second, 0.5f);
			}
		}
	);
}

void Player::RenderUINode(Renderer& renderer)
{
	renderer.UI_RENDER_FUNCTIONS().emplace_back
	(
		[&]()
		{
			for (const auto& time : m_UINode)
			{
				float pos = lerp(0.5f, 0.25f, time);
				Renderer::GetInstance().RenderImageUIPosition(m_crosshairTextureAndOffset.first, { pos, 0.5f }, m_crosshairTextureAndOffset.second, 1.0f - (time * 0.5f));
				Renderer::GetInstance().RenderImageUIPosition(m_crosshairTextureAndOffset.first, { 1.0f - pos, 0.5f }, m_crosshairTextureAndOffset.second, 1.0f - (time * 0.5f));
			}
		}
	);
}