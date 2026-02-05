#pragma once
#include "GameObjectBase.h"

class Enemy : public GameObjectBase
{
	bool m_isTutorialDummy = false;

	class Player* m_player = nullptr;

	float m_moveSpeedSquared = 4.0f;
	std::deque<DirectX::XMVECTOR> m_path = {};
	const float m_pathFindInterval = 1.0f;
	float m_pathFindTimer = 0.0f;
	float m_pathFindIntervalRandomOffset = 0.0f;

	class FSMComponentEnemy* m_fsm = nullptr;
	class ColliderComponent* m_collider = nullptr;

	float m_deathTimer = 0.0f;
	const float m_deathDuration = 2.0f;
	const float m_attackRangeSquare = 2.56f;
	float m_rotationSpeed = 10.0f;

public:
	enum class AIState
	{
		Idle,
		Chase,
		Attack,
		Dead
	};
	AIState m_state = AIState::Chase;

	Enemy() = default;
	~Enemy() override = default;
	Enemy(const Enemy&) = default;
	Enemy& operator=(const Enemy&) = default;
	Enemy(Enemy&&) = default;
	Enemy& operator=(Enemy&&) = default;

	void Die();
	void OnAttackFinished();
	class Player* GetTargetPlayer() const { return m_player; }

	void SetAsTutorialDummy();

private:
	void Initialize() override;
	void Update() override;

	void MoveAlongPath(float dt);
};