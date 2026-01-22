///BOF Enemy.cpp
#include "stdafx.h"
#include "Enemy.h"

#include "SkinnedModelComponent.h"
#include "ColliderComponent.h"
#include "FSMComponentEnemy.h"
#include "TimeManager.h"

REGISTER_TYPE(Enemy)

using namespace DirectX;


void Enemy::Initialize()
{
	m_model = CreateComponent<SkinnedModelComponent>();
	//m_model->SetModelFileName("test5.fbx");
	m_fsm = CreateComponent<FSMComponentEnemy>();
	m_collider = CreateComponent<ColliderComponent>();
	m_collider->AddBoundingBox(BoundingBox({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }));
}


void Enemy::Die()
{
	if (m_state == AIState::Dying) return;

	m_state = AIState::Dying;
	m_deathTimer = 0.0f;

	if (m_collider) m_collider->SetAlive(false);

	if (m_fsm)
	{
		m_fsm->ChangeState(FSMComponentEnemy::EDead);
	}
}

void Enemy::Update()
{
	if (m_state == AIState::Dying)
	{
		m_deathTimer += TimeManager::GetInstance().GetDeltaTime();

		if (m_deathTimer >= m_deathDuration)
		{
			SetAlive(false);
		}
	}
}
///EOF Enemy.cpp