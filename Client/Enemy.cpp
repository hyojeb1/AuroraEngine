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

REGISTER_TYPE(Enemy)

using namespace DirectX;

void Enemy::Initialize()
{
	m_model = GetComponent<SkinnedModelComponent>();
	m_fsm = GetComponent<FSMComponentEnemy>();
	m_collider = GetComponent<ColliderComponent>();

	m_player = static_cast<Player*>(SceneManager::GetInstance().GetCurrentScene()->GetGameObjectRecursive("Player"));
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
	if (m_state == AIState::Dying)
	{
		m_deathTimer += TimeManager::GetInstance().GetDeltaTime();

		if (m_deathTimer >= m_deathDuration) SetAlive(false);
	}
	LookAt(m_player->GetPosition());
	MoveDirection(TimeManager::GetInstance().GetDeltaTime() * 2.0f, Direction::Forward);
}