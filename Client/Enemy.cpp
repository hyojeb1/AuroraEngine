///BOF Enemy.cpp
#include "stdafx.h"
#include "Enemy.h"

#include "SkinnedModelComponent.h"
#include "ColliderComponent.h"
#include "FSMComponentEnemy.h"
#include "TimeManager.h"
#include "Player.h"
#include "SceneManager.h"
#include "SceneBase.h"
#include "NavigationManager.h"
#include "RNG.h"

REGISTER_TYPE(Enemy)

using namespace DirectX;

void Enemy::Initialize()
{
	m_fsm = GetComponent<FSMComponentEnemy>();
	m_collider = GetComponent<ColliderComponent>();

	m_player = static_cast<Player*>(SceneManager::GetInstance().GetCurrentScene()->GetGameObjectRecursive("Player"));
	m_pathFindIntervalRandomOffset = RANDOM(0.0f, m_pathFindInterval);
	m_pathFindTimer = -m_pathFindIntervalRandomOffset;
}

void Enemy::Die()
{
	if (m_state == AIState::Dying) return;

	m_state = AIState::Dying;
	m_deathTimer = 0.0f;

	if (m_collider) m_collider->SetAlive(false);

	if (m_fsm) m_fsm->ChangeState(FSMComponentEnemy::EDead);
}

void Enemy::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();

	if (m_state == AIState::Dying)
	{
		m_deathTimer += deltaTime;

		if (m_deathTimer >= m_deathDuration) SetAlive(false);

		return;
	}

	m_pathFindTimer += deltaTime;
	if (m_pathFindTimer >= m_pathFindInterval - m_pathFindIntervalRandomOffset)
	{
		m_path.clear();
		m_pathFindTimer = -m_pathFindIntervalRandomOffset;
	}


	MoveAlongPath();
}

void Enemy::MoveAlongPath()
{
	// 플레이어까지 경로 계산
	if (m_path.empty()) m_path = NavigationManager::GetInstance().FindPath(GetPosition(), m_player->GetPosition());

	// 경로 따라 이동
	XMVECTOR toTarget = XMVectorSubtract(m_path.front(), GetPosition());
	float distanceSquared = XMVectorGetX(XMVector3LengthSq(toTarget));
	if (distanceSquared < 0.1f * 0.1f) m_path.pop_front();
	else
	{
		XMVECTOR direction = XMVector3Normalize(toTarget);
		XMVECTOR deltaPosition = XMVectorScale(direction, m_moveSpeedSquared * TimeManager::GetInstance().GetDeltaTime());
		MovePosition(deltaPosition);
	}
}