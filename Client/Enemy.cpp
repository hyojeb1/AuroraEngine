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
#include "GameManager.h"
#include "SoundManager.h"

#include "Shared/Config/Option.h"

REGISTER_TYPE(Enemy)

using namespace std;
using namespace DirectX;

void Enemy::Initialize()
{
	m_fsm = GetComponent<FSMComponentEnemy>();
	m_collider = GetComponent<ColliderComponent>();

	m_player = static_cast<Player*>(SceneManager::GetInstance().GetCurrentScene()->GetGameObjectRecursive("Player"));
	if (!m_player) cout << "Enemy 초기화 오류: Player 게임 오브젝트를 찾을 수 없습니다." << endl;

	m_pathFindIntervalRandomOffset = RANDOM(0.0f, m_pathFindInterval);
	m_pathFindTimer = -m_pathFindIntervalRandomOffset;
}

void Enemy::Die()
{
	if (m_state == AIState::Dead) return;

	m_state = AIState::Dead;
	m_deathTimer = 0.0f;

	if (m_collider) m_collider->SetAlive(false);

	if (m_fsm) m_fsm->ChangeState(FSMComponentEnemy::EDead);
	
	GameManager::GetInstance().AddKill();

	SoundManager::GetInstance().SFX_Shot(GetPosition(), Config::Enemy_Die);
}

void Enemy::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();

	switch (m_state) {
	case Enemy::AIState::Idle: 
		//1. 튜토리얼 허수아비용 
		//2. Attack후 잠깐 후딜용으로 쓰기 (이건 애니메이션 받고 테스트해봐야 함)
		break;
	case Enemy::AIState::Chase:
		if(m_player){
			XMVECTOR toPlayer = m_player->GetPosition() - GetPosition();
			float distSq = XMVectorGetX(XMVector3LengthSq(toPlayer));

			if (distSq <= m_attackRangeSquare) {
				m_state = AIState::Attack;
				if (m_fsm) m_fsm->ChangeState(FSMComponentEnemy::EAttack);
				return;
			}

			m_pathFindTimer += deltaTime;
			if (m_pathFindTimer >= m_pathFindInterval - m_pathFindIntervalRandomOffset) { m_path.clear(); m_pathFindTimer = -m_pathFindIntervalRandomOffset; }
			MoveAlongPath();
		}
		break;
	case Enemy::AIState::Attack: // 여기서 업데이트 안하고 FSMComponent에서 애니메이션을 진행함
		return;
	case Enemy::AIState::Dead:
		m_deathTimer += deltaTime;
		if (m_deathTimer >= m_deathDuration) SetAlive(false);
		return;
	default:
		break;
	}
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

void Enemy::OnAttackFinished()
{
	if (m_state == AIState::Dead) return;

	// 공격이 끝났으니 다시 추적 상태로 복귀
	// (만약 튜토리얼용 허수아비라면 여기서 Idle로 보내는 분기 추가 가능)
	m_state = AIState::Chase;

	if (m_fsm) m_fsm->ChangeState(FSMComponentEnemy::EChase);
}