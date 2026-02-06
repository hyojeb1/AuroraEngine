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
#include "NavigationManager.h"

#include "FSMComponentGun.h"

#include "Shared/Config/Option.h"

REGISTER_TYPE(Player)

using namespace std;
using namespace DirectX;

void Player::Initialize()
{
	XMStoreFloat3(&m_playerRotation, GetRotation());

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

	UpdateRotation(input, deltaTime);
	UpdateMoveDirection(input);
	
	if (input.GetKeyDown(KeyCode::MouseLeft) && m_bulletCnt > 0 && sm.CheckRhythm(Config::InputCorrection) < InputType::Miss) PlayerShoot();
	if (!m_isDashing && input.GetKeyDown(KeyCode::Space) && sm.CheckRhythm(Config::InputCorrection) < InputType::Miss) PlayerTriggerDash();
	if (!m_isDeadEyeActive && input.GetKeyDown(KeyCode::MouseRight) && sm.CheckRhythm(Config::InputCorrection) < InputType::Miss) PlayerDeadEyeStart();

	if (m_isDeadEyeActive) PlayerDeadEye(deltaTime, input);

	XMVECTOR previousPosition = GetPosition();

	if (m_isDashing) PlayerDash(deltaTime);
	else MovePosition(m_normalizedMoveDirection * m_moveSpeed * deltaTime); // ?���? ???직임

	// 만약 이동한 위치가 네비게이션 메시 밖이면 이전 위치로 되돌림 // 월드 좌표계는 나중에 업데이트 됨으로 로컬 좌표계로 해햐함
	if (NavigationManager::GetInstance().FindNearestPoly(GetPosition(), 1.0f) < 0) SetPosition(previousPosition);

	if (input.GetKeyDown(KeyCode::R))
	{
		switch (sm.CheckRhythm(Config::InputCorrection))
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

void Player::UpdateRotation(InputManager& input, float deltaTime)
{
	const POINT& mouseDelta = input.GetMouseDelta();

	m_playerRotation.x += static_cast<float>(mouseDelta.y) * m_cameraSensitivity;
	m_playerRotation.y += static_cast<float>(mouseDelta.x) * m_cameraSensitivity;

	constexpr float LIMIT = 90.0f - 1.0f;
	if (m_playerRotation.x > LIMIT) m_playerRotation.x = LIMIT;
	if (m_playerRotation.x < -LIMIT) m_playerRotation.x = -LIMIT;

	SetRotation({ m_playerRotation.x, m_playerRotation.y, 0.0f, 0.0f });

	m_playerRotation.z = lerp(m_playerRotation.z, 0.0f, deltaTime * 5.0f);
	Rotate({ 0.0f, 0.0f, m_playerRotation.z, 0.0f });
}

void Player::UpdateMoveDirection(InputManager& input)
{
	m_inputDirection =
	{
		static_cast<float>(input.GetKey(KeyCode::D)) - static_cast<float>(input.GetKey(KeyCode::A)),
		0.0f,
		static_cast<float>(input.GetKey(KeyCode::W)) - static_cast<float>(input.GetKey(KeyCode::S)),
		0.0f
	};

	XMVECTOR yawQuaternion = XMQuaternionRotationRollPitchYaw(0.0f, m_playerRotation.y * DEG_TO_RAD, 0.0f);
	m_normalizedMoveDirection = XMVector3Normalize(XMVector3Rotate(m_inputDirection, yawQuaternion));
}

void Player::PlayerTriggerDash()
{
	if (XMVectorGetX(XMVector3LengthSq(m_normalizedMoveDirection)) <= numeric_limits<float>::epsilon()) return;

	m_isDashing = true;
	m_dashTimer = m_kDashDuration;
	m_dashDirection = m_normalizedMoveDirection;
	const float inputX = XMVectorGetX(XMVector3Normalize(m_inputDirection));

	// 카메?�� ?��?��
	m_playerRotation.z = -Config::Player_Dash_Tilt * inputX;

	// Radial Blur ?��?��
	SceneBase::SetRadialBlurCenter({ inputX * 0.5f + 0.5f, 0.5f });
	SceneBase::SetRadialBlurDist(0.33f);
	SceneBase::SetRadialBlurStrength(1.7f);
	SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::RadialBlur, true);

	// ?��?��?�� ?��기다 ?��?��?�� ?��
	SoundManager::GetInstance().SFX_Shot(GetPosition(), Config::Player_Dash);
}

void Player::PlayerDash(float deltaTime)
{
	m_dashTimer -= deltaTime;
	MovePosition(m_dashDirection * m_kDashSpeed * deltaTime);
	if (m_dashTimer <= 0.0f) m_isDashing = false;

	float t = 1.0f - (m_dashTimer / m_kDashDuration); // 0->1
	float smooth = t * t * (3.0f - 2.0f * t); // smoothstep
	SceneBase::SetRadialBlurStrength(8.0f * (1.0f - smooth)); // ?��?�� ?��?���?

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
	SoundManager::GetInstance().UI_Shot(Config::Player_Shoot);

	const XMVECTOR& origin = GetPosition();
	const XMVECTOR& direction = GetWorldDirectionVector(Direction::Forward);
	float distance = 0.0f;
	GameObjectBase* hit = ColliderComponent::CheckCollision(origin, direction, distance);
	if (!hit) distance = 100.0f;
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

	SoundManager::GetInstance().UI_Shot(Config::Player_Reload_Spin);

	int reloadCount = Config::Player_Reload_Cocking_Count + cnt;
	SoundManager::GetInstance().AddNodeDestroyedListenerOnce([this, cnt = reloadCount]()mutable ->bool
		{
			if (--cnt > 0)
			{
				return false;
			}
			SoundManager::GetInstance().UI_Shot(Config::Player_Reload_Cocking);
			return true;
		});
}

void Player::PlayerDeadEyeStart()
{
	vector<GameObjectBase*> hits = ColliderComponent::CheckCollision(m_cameraComponent->GetBoundingFrustum());
	if (hits.empty()) return;

	bool hasEnemy = false;

	for (GameObjectBase* hit : hits)
	{
		if (Enemy* enemy = dynamic_cast<Enemy*>(hit))
		{
			// ?��?��?�� ?��?��물이 ?��?���? ?��?��
			float distance = 0.0f;
			const XMVECTOR& origin = GetPosition();
			const XMVECTOR& targetPos = XMVectorAdd(enemy->GetWorldPosition(), { 0.0f, 1.0f, 0.0f, 0.0f }); // ?�� 충심?�� y = 0.0f?��?�� ?���? ?���?
			if (!dynamic_cast<Enemy*>(ColliderComponent::CheckCollision(origin, XMVectorSubtract(targetPos, origin), distance))) continue;

			hasEnemy = true;
			XMFLOAT2 distancePair = Renderer::GetInstance().ToUIPosition(m_cameraComponent->WorldToScreenPosition(targetPos));
			m_deadEyeTargets.emplace_back(powf(distancePair.x - 0.5f, 2) + powf(distancePair.y - 0.5f, 2), enemy);
		}
	}
	if (hasEnemy)
	{
		m_isDeadEyeActive = true;

		TimeManager::GetInstance().SetTimeScale(0.1f);

		m_cameraSensitivity = 0.01f;

		SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::Grayscale, true);
		//SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::RadialBlur, true);
		//SceneBase::SetRadialBlurDist(0.33f);

		sort(m_deadEyeTargets.begin(), m_deadEyeTargets.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
		if (m_deadEyeTargets.size() > 6) m_deadEyeTargets.resize(6);
		sort(m_deadEyeTargets.begin(), m_deadEyeTargets.end(), [&](const auto& a, const auto& b) { return m_cameraComponent->WorldToScreenPosition(a.second->GetWorldPosition()).x > m_cameraComponent->WorldToScreenPosition(b.second->GetWorldPosition()).x; });

		SoundManager::GetInstance().ChangeLowpass();

		m_currentNodeIndex = SoundManager::GetInstance().GetRhythmTimerIndex();
		m_DeadEyeCount = m_deadEyeTargets.size();
		m_deadEyeTotalDuration = static_cast<float>(m_DeadEyeCount) * 0.5f;
		m_deadEyeDuration = 0.0f;

		m_prevDeadEyePos = m_cameraComponent->WorldToScreenPosition(m_deadEyeTargets.back().second->GetWorldPosition());
		m_nextDeadEyePos = m_prevDeadEyePos;
	}
}

void Player::PlayerDeadEye(float deltaTime, InputManager& input)
{
	if (m_deadEyeTargets.empty()) { PlayerDeadEyeEnd(); return; }

	m_deadEyeDuration += deltaTime;
	m_deadEyeMoveTimer += deltaTime * m_deadEyeMoveSpeed;

	float effectIntensity = min((m_deadEyeDuration / m_deadEyeTotalDuration) * 16.0f, 1.0f);
	SceneBase::SetGrayScaleIntensity(effectIntensity);
	//SceneBase::SetRadialBlurStrength(effectIntensity * 2.5f);

	const XMVECTOR& targetPos = m_deadEyeTargets.back().second->GetWorldPosition();
	m_nextDeadEyePos = m_cameraComponent->WorldToScreenPosition(targetPos);
	//SceneBase::SetRadialBlurCenter(Renderer::GetInstance().ToUIPosition(m_nextDeadEyePos));

	if (input.GetKeyDown(KeyCode::MouseLeft))
	{
		m_prevDeadEyePos = m_nextDeadEyePos;
		m_deadEyeTargets.back().second->Die();
		if (m_deadEyeTargets.size() > 1)
		{
			m_nextDeadEyePos = m_cameraComponent->WorldToScreenPosition(m_deadEyeTargets[m_deadEyeTargets.size() - 2].second->GetWorldPosition());
			m_deadEyeMoveTimer = 0.0f;
		}

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
	//SceneBase::SetPostProcessingFlag(PostProcessingBuffer::PostProcessingFlag::RadialBlur, true);
	//SceneBase::SetRadialBlurStrength(0.0f);

	SoundManager::GetInstance().ChangeLowpass();

	//TriggerLUT(); // 맛이 ?��?��
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
					lerp(m_prevDeadEyePos.x, m_nextDeadEyePos.x, min(m_deadEyeMoveTimer, 1.0f)),
					lerp(m_prevDeadEyePos.y, m_nextDeadEyePos.y, min(m_deadEyeMoveTimer, 1.0f))
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

	// smoothstep (깔끔?�� �??��/감속)
	float smooth = t * t * (3.0f - 2.0f * t);
	float factor = m_lutCrossfadeReverse ? (1.0f - smooth) : smooth;

	SceneBase::SetLutLerpFactor(factor);

	if (t >= 1.0f)
	{
		if (!m_lutCrossfadeReverse)
		{
			// ?��방향 ?��?��
			m_lutCrossfadeReverse = true;
			m_lutCrossfadeElapsed = 0.0f;
		}
		else
		{
			// 종료: flag off + ?���?
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
