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

	m_crosshairSRV = resourceManager.GetTexture("Crosshair.png");

	// srv에서 크기 정보 얻기
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	m_crosshairSRV->GetDesc(&srvDesc);
	if (srvDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D)
	{
		D3D11_TEX2D_SRV tex2DSRV = srvDesc.Texture2D;
		com_ptr<ID3D11Resource> resource = nullptr;
		m_crosshairSRV->GetResource(resource.GetAddressOf());
		com_ptr<ID3D11Texture2D> texture2D = nullptr;
		resource.As(&texture2D);
		D3D11_TEXTURE2D_DESC textureDesc = {};
		texture2D->GetDesc(&textureDesc);
		m_crosshairOffset.x = static_cast<float>(textureDesc.Width) * 0.5f;
		m_crosshairOffset.y = static_cast<float>(textureDesc.Height) * 0.5f;
	}

	m_cameraObject = GetChildGameObject<CamRotObject>("CamRotObject_2");
	m_gunObject = m_cameraObject->GetChildGameObject("Gun");
	m_gunObject->CreateComponent<FSMComponentGun>();
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

		if (m_gunObject)
		{
			auto gunFSM = m_gunObject->GetComponent<FSMComponentGun>();
			if (gunFSM)
			{
				gunFSM->Fire();
			}
		}

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
	if (!m_isDeadEyeActive && input.GetKeyDown(KeyCode::MouseRight))
	{
		const DXGI_SWAP_CHAIN_DESC1& swapChainDesc = Renderer::GetInstance().GetSwapChainDesc();
		float halfWidth = static_cast<float>(swapChainDesc.Width / 2);
		float halfHeight = static_cast<float>(swapChainDesc.Height / 2);

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
			m_cameraSensitivity = m_cameraObject->GetSensitivity();
			m_cameraObject->SetSensitivity(0.01f); // 데드 아이 타임 활성화 시 카메라 감도 감소
			m_postProcessingBuffer.flags |= static_cast<UINT>(PostProcessingBuffer::PostProcessingFlag::Grayscale);

			sort(m_deadEyeTargets.begin(), m_deadEyeTargets.end(), [](const auto& a, const auto& b) { return get<0>(a) < get<0>(b); });
			if (m_deadEyeTargets.size() > 6) m_deadEyeTargets.resize(6);
		}
	}

	if (m_isDeadEyeActive)
	{
		m_postProcessingBuffer.grayScaleIntensity = (1.0f - (m_deadEyeTime / m_deadEyeDuration)) * 2.0f;
		m_deadEyeTime -= deltaTime;

		if (m_deadEyeTime <= 0.0f) // 데드 아이 타임 종료
		{
			for (const auto& [timing, enemy] : m_deadEyeTargets)
			{
				if (timing > 999990.1f) continue; // 0.1초 이상 타이밍이 안맞으면 무시

				enemy->SetAlive(false);

				LineBuffer lineBuffer = {};
				if (m_gunObject) XMStoreFloat4(&lineBuffer.linePoints[0], m_gunObject->GetWorldPosition());
				lineBuffer.lineColors[0] = XMFLOAT4{ 1.0f, 0.5f, 0.5f, 1.0f };
				XMStoreFloat4(&lineBuffer.linePoints[1], enemy->GetWorldPosition());
				lineBuffer.lineColors[1] = XMFLOAT4{ 1.0f, 0.5f, 0.5f, 1.0f };

				m_lineBuffers.emplace_back(lineBuffer, 0.5f);
			}

			if (m_gunObject)
			{
				auto gunFSM = m_gunObject->GetComponent<FSMComponentGun>();
				if (gunFSM)
				{
					gunFSM->Fire();
				}
			}

			m_isDeadEyeActive = false;

			TimeManager::GetInstance().SetTimeScale(1.0f); // 시간 정상화
			m_cameraObject->SetSensitivity(m_cameraSensitivity); // 카메라 감도 원래대로
			m_deadEyeTargets.clear();
			m_postProcessingBuffer.flags &= ~static_cast<UINT>(PostProcessingBuffer::PostProcessingFlag::Grayscale);
			m_postProcessingBuffer.grayScaleIntensity = 0.0f;
		}

		Renderer::GetInstance().GetDeviceContext()->UpdateSubresource(ResourceManager::GetInstance().GetConstantBuffer(PSConstBuffers::PostProcessing).Get(), 0, nullptr, &m_postProcessingBuffer, 0, 0);
	}

	for_each(m_lineBuffers.begin(), m_lineBuffers.end(), [&](auto& pair) { pair.second -= deltaTime; });
	if (!m_lineBuffers.empty() && m_lineBuffers.front().second < 0.0f) m_lineBuffers.pop_front();
}

void Player::Render()
{
	Renderer& renderer = Renderer::GetInstance();

	// 크로스헤어 UI 렌더링
	renderer.UI_RENDER_FUNCTIONS().emplace_back
	(
		[&]() { Renderer::GetInstance().RenderImageUIPosition(m_crosshairSRV, { 0.5f, 0.5f }, m_crosshairOffset); }
	);

	if (!m_lineBuffers.empty())
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

	if (!m_deadEyeTargets.empty())
	{
		renderer.UI_RENDER_FUNCTIONS().emplace_back
		(
			[&]()
			{
				const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

				for (const auto& [timing, enemy] : m_deadEyeTargets)
				{
					Renderer::GetInstance().RenderImageScreenPosition(m_crosshairSRV, mainCamera.WorldToScreenPosition(enemy->GetWorldPosition()), m_crosshairOffset, 0.5f);
				}
			}
		);
	}
}

void Player::Finalize()
{
	TimeManager::GetInstance().SetTimeScale(1.0f); // 시간 느린 상태에서 종료되는 상황 방지
}