#pragma once
#include "GameObjectBase.h"

class ParticleObject : public GameObjectBase
{
	float m_lifetime = 1.0f;

public:
	ParticleObject() = default;
	~ParticleObject() override = default;
	ParticleObject(const ParticleObject&) = default;
	ParticleObject& operator=(const ParticleObject&) = default;
	ParticleObject(ParticleObject&&) = default;
	ParticleObject& operator=(ParticleObject&&) = default;

	void SetLifetime(float lifetime) { m_lifetime = lifetime; }

private:
	void Initialize() override;
	void Update() override;
};