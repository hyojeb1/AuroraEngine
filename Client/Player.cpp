#include "stdafx.h"
#include "Player.h"

#include "Renderer.h"
#include "SoundManager.h"
#include "TimeManager.h"
#include "InputManager.h"
#include "ColliderComponent.h"
#include "CameraComponent.h"
#include "ResourceManager.h"
#include "ModelComponent.h"
#include "SceneBase.h"
#include "Enemy.h"
#include "ParticleObject.h"

#include "FSMComponentGun.h"

REGISTER_TYPE(Player)

using namespace std;
using namespace DirectX;

void Player::Initialize()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	m_lineVertexBufferAndShader = resourceManager.GetVertexShaderAndInputLayout("VSLine.hlsl");
	m_linePixelShader = resourceManager.GetPixelShader("PSColor.hlsl");

	m_cameraComponent = GetComponent<CameraComponent>();
	m_cameraComponent->SetAsMainCamera();
	m_gunObject = GetChildGameObject("Gun");
	m_gunFSM = m_gunObject->CreateComponent<FSMComponentGun>();

	m_deadEyeTextureAndOffset = resourceManager.GetTextureAndOffset("Crosshair.png");
	m_enemyHitTextureAndOffset = resourceManager.GetTextureAndOffset("CrosshairHit.png");

	m_bulletImgs = resourceManager.GetTextureAndOffset("bullet.png");

	m_DeadEyeCount = 4;
	m_bulletCnt = 6;

	m_bulletUIpos = { 0.82f,0.9f };
	m_bulletInterval = 0.03f;
}

void Player::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();
	InputManager& input = InputManager::GetInstance();
	auto& sm = SoundManager::GetInstance();

	UpdateMoveDirection(input);

	if (input.GetKeyDown(KeyCode::Space) && !m_isDashing && sm.CheckRhythm(0.1f) < InputType::Miss) PlayerTriggerDash(input);

	if (m_isDashing) PlayerDash(deltaTime, input);
	else MovePosition(m_normalizedMoveDirection * m_moveSpeed * deltaTime); // 일반 움직임

	if (input.GetKeyDown(KeyCode::MouseLeft) && m_bulletCnt > 0 && sm.CheckRhythm(0.1f) < InputType::Miss) PlayerShoot();
	if (!m_isDeadEyeActive && input.GetKeyDown(KeyCode::MouseRight) && sm.CheckRhythm(0.1f) < InputType::Miss) PlayerDeadEyeStart();
	if (input.GetKeyDown(KeyCode::R))
	{
		switch (sm.CheckRhythm(0.1f))
		{
		case InputType::Early:
			PlayerReload(1);
			break;
		case InputType::Perfect:
			PlayerReload(0);
			break;
		case InputType::Late:
			PlayerReload(0);
			break;
		}
	}

	if (m_isDeadEyeActive) PlayerDeadEye(deltaTime, input);

	for_each(m_lineBuffers.begin(), m_lineBuffers.end(), [&](auto& pair) { pair.second -= deltaTime; });
	if (!m_lineBuffers.empty() && m_lineBuffers.front().second < 0.0f) m_lineBuffers.pop_front();
	if (m_enemyHitTimer > 0.0f) m_enemyHitTimer -= deltaTime;

	if (input.GetKey(KeyCode::LeftBracket))	{ m_bulletUIpos.first += 0.1f * deltaTime; }
	if (input.GetKey(KeyCode::RightBracket)) { m_bulletUIpos.second += 0.1f * deltaTime; }
	if (input.GetKey(KeyCode::Num0)) { m_bulletInterval += 0.01f * deltaTime; }

	UpdateLutCrossfade(deltaTime);
}

void Player::Render()
{
	Renderer& renderer = Renderer::GetInstance();

	if (!m_lineBuffers.empty()) RenderLineBuffers(renderer);
	if (!m_deadEyeTargets.empty()) RenderDeadEyeTargetsUI(renderer);
	if (m_enemyHitTimer > 0.0f) RenderEnemyHitUI(renderer);
	if (m_bulletCnt <= m_MaxBullet && m_bulletCnt != 0) RenderBullets(renderer);
}

void Player::Finalize()
{
	TimeManager::GetInstance().SetTimeScale(1.0f);
}

void Player::UpdateMoveDirection(InputManager& input)
{
	const POINT& mouseDelta = input.GetMouseDelta();
	const XMVECTOR& currentRotation = GetRotation();

	float pitch = XMVectorGetX(currentRotation) + static_cast<float>(mouseDelta.y) * m_cameraSensitivity;
	float yaw = XMVectorGetY(currentRotation) + static_cast<float>(mouseDelta.x) * m_cameraSensitivity;

	constexpr float LIMIT = 90.0f - 1.0f;
	if (pitch > LIMIT) pitch = LIMIT;
	if (pitch < -LIMIT) pitch = -LIMIT;

	SetRotation({ pitch, yaw, 0.0f, 0.0f });

	m_inputDirection = XMVectorZero();
	XMVectorSetZ(m_inputDirection, static_cast<float>(input.GetKey(KeyCode::W)) - static_cast<float>(input.GetKey(KeyCode::S)));
	XMVectorSetX(m_inputDirection, static_cast<float>(input.GetKey(KeyCode::D)) - static_cast<float>(input.GetKey(KeyCode::A)));

	XMVECTOR yawQuaternion = XMQuaternionRotationRollPitchYaw(0.0f, yaw * DEG_TO_RAD, 0.0f);
	m_normalizedMoveDirection = XMVector3Normalize(XMVector3Rotate(m_inputDirection, yawQuaternion));
}

void Player::PlayerTriggerDash(InputManager& input)
{
	m_isDashing = true;
	m_dashTimer = m_kDashDuration;
	m_dashDirection = m_normalizedMoveDirection;

	XMFLOAT2 blurCenter = { XMVectorGetX(m_normalizedMoveDirection) * 0.5f + 0.5f, 0.5f };



	SceneBase::SetRadialBlurCenter({ XMVectorGetX(m_normalizedMoveDirection) * 0.5f + 0.5f, 0.5f });
	SceneBase::SetRadialBlurDist(0.33f);
	SceneBase::SetRadialBlurStrength(1.7f);
	SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::RadialBlur, true);

	// 사운드 여기다 넣아야 함
}

void Player::PlayerDash(float deltaTime, InputManager& input)
{
	m_dashTimer -= deltaTime;
	MovePosition(m_dashDirection * m_kDashSpeed * deltaTime);
	if (m_dashTimer <= 0.0f) { m_isDashing = false; }

	float t = 1.0f - (m_dashTimer / m_kDashDuration); // 0->1
	float smooth = t * t * (3.0f - 2.0f * t);         // smoothstep
	SceneBase::SetRadialBlurStrength(8.0f * (1.0f - smooth)); // 점점 약해짐

	// 카메라 회전

	if (m_dashTimer <= 0.0f)
	{
		m_isDashing = false;
		SceneBase::SetRadialBlurStrength(0.0f);
		SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::RadialBlur, false);
	}

}

void Player::PlayerShoot()
{
	if (!m_gunObject || m_isDeadEyeActive) return;

	if (m_gunFSM) m_gunFSM->Fire();
	--m_bulletCnt;
	SoundManager::GetInstance().SFX_Shot(GetPosition(), Config::Player_Shoot);

	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();

	const XMVECTOR& origin = mainCamera.GetPosition();
	const XMVECTOR& direction = mainCamera.GetForwardVector();
	float distance = 0.0f;
	GameObjectBase* hit = ColliderComponent::CheckCollision(origin, direction, distance);
	const XMVECTOR& hitPosition = XMVectorAdd(origin, XMVectorScale(direction, distance));

	ParticleObject* smoke = dynamic_cast<ParticleObject*>(CreatePrefabChildGameObject("Smoke.json"));
	const XMVECTOR& gunPos = m_gunObject->GetWorldPosition();
	smoke->SetPosition(gunPos);
	smoke->SetScale({ 1.0f, 1.0f, distance, 1.0f });
	smoke->LookAt(hitPosition);
	smoke->SetLifetime(5.0f);

	if (Enemy* enemy = dynamic_cast<Enemy*>(hit))
	{
		enemy->Die();
		ParticleObject* gem = dynamic_cast<ParticleObject*>(CreatePrefabChildGameObject("Gem.json"));
		gem->SetPosition(hitPosition);
		gem->SetLifetime(5.0f);

		m_enemyHitTimer = m_enemyHitDisplayTime;

		TriggerLUT();
	}
}

void Player::PlayerReload(int cnt)
{
	if (m_bulletCnt == m_MaxBullet) return;

	//Reload Anime + rhythm check

	m_bulletCnt = m_MaxBullet;

	SoundManager::GetInstance().SFX_Shot(GetPosition(), Config::Player_Reload_Spin);

	int reloadCount = Config::Player_Reload_Cocking_Count + cnt;
	SoundManager::GetInstance().AddNodeDestroyedListenerOnce([this, cnt = reloadCount]()mutable ->bool
		{
			if (--cnt > 0)
			{
				return false;
			}
			SoundManager::GetInstance().SFX_Shot(GetPosition(), Config::Player_Reload_Cocking);
			return true;
		});
}

void Player::PlayerDeadEyeStart()
{
	const DXGI_SWAP_CHAIN_DESC1& swapChainDesc = Renderer::GetInstance().GetSwapChainDesc();
	float halfWidth = static_cast<float>(swapChainDesc.Width) * 0.5f;
	float halfHeight = static_cast<float>(swapChainDesc.Height) * 0.5f;

	const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
	vector<GameObjectBase*> hits = ColliderComponent::CheckCollision(mainCamera.GetBoundingFrustum());
	if (hits.empty()) return;

	bool hasEnemy = false;

	for (GameObjectBase* hit : hits)
	{
		if (Enemy* enemy = dynamic_cast<Enemy*>(hit))
		{
			// 사이에 장애물이 있는지 확인
			float distance = 0.0f;
			const XMVECTOR& origin = mainCamera.GetPosition();
			const XMVECTOR& targetPos = XMVectorAdd(enemy->GetWorldPosition(), { 0.0f, 1.0f, 0.0f, 0.0f }); // 적 충심이 y = 0.0f여서 약간 올림
			if (!dynamic_cast<Enemy*>(ColliderComponent::CheckCollision(origin, XMVectorSubtract(targetPos, origin), distance))) continue;

			hasEnemy = true;
			XMFLOAT2 distancePair = mainCamera.WorldToScreenPosition(targetPos);
			m_deadEyeTargets.emplace_back(powf(distancePair.x - halfWidth, 2) + powf(distancePair.y - halfHeight, 2), enemy);
		}
	}
	if (hasEnemy)
	{
		m_isDeadEyeActive = true;

		TimeManager::GetInstance().SetTimeScale(0.1f);

		m_cameraSensitivity = 0.01f;

		SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::Grayscale, true);

		sort(m_deadEyeTargets.begin(), m_deadEyeTargets.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
		if (m_deadEyeTargets.size() > 6) m_deadEyeTargets.resize(6);
		sort(m_deadEyeTargets.begin(), m_deadEyeTargets.end(), [&](const auto& a, const auto& b) { return mainCamera.WorldToScreenPosition(a.second->GetWorldPosition()).x > mainCamera.WorldToScreenPosition(b.second->GetWorldPosition()).x; });

		SoundManager::GetInstance().ChangeLowpass();

		m_currentNodeIndex = SoundManager::GetInstance().GetRhythmTimerIndex();
		m_DeadEyeCount = m_deadEyeTargets.size();
		m_deadEyeTotalDuration = static_cast<float>(m_DeadEyeCount) * 0.5f;
		m_deadEyeDuration = 0.0f;

		m_prevDeadEyePos = CameraComponent::GetMainCamera().WorldToScreenPosition(m_deadEyeTargets.back().second->GetWorldPosition());
		m_nextDeadEyePos = m_prevDeadEyePos;
	}
}

void Player::PlayerDeadEye(float deltaTime, InputManager& input)
{
	if (m_deadEyeTargets.empty()) { PlayerDeadEyeEnd(); return; }

	m_deadEyeDuration += deltaTime;
	m_deadEyeMoveTimer += deltaTime * m_deadEyeMoveSpeed;
	SceneBase::SetGrayScaleIntensity((m_deadEyeDuration / m_deadEyeTotalDuration) * 16.0f);

	if (input.GetKeyDown(KeyCode::MouseLeft))
	{
		const XMVECTOR& targetPos = m_deadEyeTargets.back().second->GetWorldPosition();

		m_prevDeadEyePos = CameraComponent::GetMainCamera().WorldToScreenPosition(targetPos);
		m_deadEyeTargets.back().second->Die();
		if (m_deadEyeTargets.size() > 1)
		{
			m_nextDeadEyePos = CameraComponent::GetMainCamera().WorldToScreenPosition(m_deadEyeTargets[m_deadEyeTargets.size() - 2].second->GetWorldPosition());
			m_deadEyeMoveTimer = 0.0f;
		}
		else m_nextDeadEyePos = m_prevDeadEyePos;

		const XMVECTOR& gunPos = m_gunObject->GetWorldPosition();
		ParticleObject* smoke = dynamic_cast<ParticleObject*>(CreatePrefabChildGameObject("Smoke.json"));
		smoke->SetPosition(gunPos);
		smoke->SetScale({ 1.0f, 1.0f, XMVectorGetX(XMVector3LengthEst(XMVectorSubtract(gunPos, targetPos))), 1.0f });
		smoke->LookAt(targetPos);
		smoke->SetLifetime(5.0f);

		ParticleObject* gem = dynamic_cast<ParticleObject*>(CreatePrefabChildGameObject("Gem.json"));
		gem->SetPosition(targetPos);
		gem->SetLifetime(5.0f);

		m_deadEyeTargets.pop_back();
	}

	if (SoundManager::GetInstance().GetRhythmTimerIndex() >= m_currentNodeIndex + m_DeadEyeCount) PlayerDeadEyeEnd();
}

void Player::PlayerDeadEyeEnd()
{
	if (m_gunObject) if (m_gunFSM) m_gunFSM->Fire();

	m_isDeadEyeActive = false;

	TimeManager::GetInstance().SetTimeScale(1.0f);

	m_cameraSensitivity = 0.1f;

	m_deadEyeTargets.clear();

	SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::Grayscale, false);
	SceneBase::SetGrayScaleIntensity(0.0f);

	SoundManager::GetInstance().ChangeLowpass();

	//TriggerLUT(); // 맛이 없음
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
			Renderer::GetInstance().RenderImageScreenPosition
			(
				m_deadEyeTextureAndOffset.first,
				{
					m_prevDeadEyePos.x + (m_nextDeadEyePos.x - m_prevDeadEyePos.x) * min(m_deadEyeMoveTimer, 1.0f),
					m_prevDeadEyePos.y + (m_nextDeadEyePos.y - m_prevDeadEyePos.y) * min(m_deadEyeMoveTimer, 1.0f)
				},
				m_deadEyeTextureAndOffset.second, 0.5f
			);
		}
	);
}

void Player::RenderEnemyHitUI(Renderer& renderer)
{
	renderer.UI_RENDER_FUNCTIONS().emplace_back
	(
		[&]()
		{
			const DXGI_SWAP_CHAIN_DESC1& swapChainDesc = Renderer::GetInstance().GetSwapChainDesc();
			Renderer::GetInstance().RenderImageUIPosition(m_enemyHitTextureAndOffset.first, { 0.5f, 0.5f }, m_enemyHitTextureAndOffset.second, 0.5f);
		}
	);
}

void Player::RenderBullets(class Renderer& renderer)
{
	float pos;
	for (int i = 0; i < m_bulletCnt; i++)
	{
		pos = m_bulletUIpos.first + (m_bulletInterval * i);
		renderer.UI_RENDER_FUNCTIONS().emplace_back([&,pos]() { Renderer::GetInstance().RenderImageUIPosition(m_bulletImgs.first, { pos, m_bulletUIpos.second }, m_bulletImgs.second, 0.1f); });
	}
}

void Player::UpdateLutCrossfade(float deltaTime)
{
	if (!m_lutCrossfadeActive) return;

	m_lutCrossfadeElapsed += deltaTime;
	float t = m_lutCrossfadeElapsed / m_lutCrossfadeDuration;
	if (t > 1.0f) t = 1.0f;

	// smoothstep (깔끔한 가속/감속)
	float smooth = t * t * (3.0f - 2.0f * t);
	float factor = m_lutCrossfadeReverse ? (1.0f - smooth) : smooth;

	SceneBase::SetLutLerpFactor(factor);

	if (t >= 1.0f)
	{
		if (!m_lutCrossfadeReverse)
		{
			// 역방향 시작
			m_lutCrossfadeReverse = true;
			m_lutCrossfadeElapsed = 0.0f;
		}
		else
		{
			// 종료: flag off + 원복
			m_lutCrossfadeActive = false;
			m_lutCrossfadeReverse = false;
			SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::LUT_CROSSFADE, false);
			SceneBase::SetLutLerpFactor(0.0f);
		}
	}
}

void Player::TriggerLUT()
{
	m_lutCrossfadeActive = true;
	m_lutCrossfadeReverse = false;
	m_lutCrossfadeElapsed = 0.0f;
	SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::LUT_CROSSFADE, true);
	SceneBase::SetLutLerpFactor(0.0f);
}
