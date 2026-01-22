///BOF Enemy.h
#pragma once
#include "GameObjectBase.h"

class Enemy : public GameObjectBase
{
public:
	enum class AIState
	{
		Alive,
		Dying 
	};

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

private:
	class SkinnedModelComponent* m_model = nullptr;
	class FSMComponentEnemy* m_fsm = nullptr;
	class ColliderComponent* m_collider = nullptr;

	AIState m_state = AIState::Alive;

	float m_deathTimer = 0.0f;
	const float m_deathDuration = 2.0f;
};
///EOF Enemy.h