#pragma once
#include "GameObjectBase.h"

class Enemy : public GameObjectBase
{
	class Player* m_player = nullptr;

	float m_moveSpeedSquared = 4.0f;
	std::deque<DirectX::XMVECTOR> m_path = {};
	const float m_pathFindInterval = 1.0f;
	float m_pathFindTimer = 0.0f;
	float m_pathFindIntervalRandomOffset = 0.0f;

	class FSMComponentEnemy* m_fsm = nullptr;
	class ColliderComponent* m_collider = nullptr;

	enum class AIState
	{
		Idle,

		Dying
	};
	AIState m_state = AIState::Idle;

	float m_deathTimer = 0.0f;
	const float m_deathDuration = 2.0f;

public:
	Enemy() = default;
	~Enemy() override = default;
	Enemy(const Enemy&) = default;
	Enemy& operator=(const Enemy&) = default;
	Enemy(Enemy&&) = default;
	Enemy& operator=(Enemy&&) = default;

	void Die();

private:
	void Initialize() override;
	void Update() override;

	void MoveAlongPath();
};