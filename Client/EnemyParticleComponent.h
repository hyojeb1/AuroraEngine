#pragma once
#include "ParticleComponent.h"

class EnemyParticleComponent : public ParticleComponent
{
public:
	EnemyParticleComponent() = default;
	~EnemyParticleComponent() = default;
	EnemyParticleComponent(const EnemyParticleComponent&) = default;
	EnemyParticleComponent& operator=(const EnemyParticleComponent&) = default;
	EnemyParticleComponent(EnemyParticleComponent&&) = default;
	EnemyParticleComponent& operator=(EnemyParticleComponent&&) = default;

private:
	void Initialize() override;
};