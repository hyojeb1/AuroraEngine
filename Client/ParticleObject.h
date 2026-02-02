#pragma once
#include "GameObjectBase.h"

class ParticleObject : public GameObjectBase
{
public:
	ParticleObject() = default;
	~ParticleObject() override = default;
	ParticleObject(const ParticleObject&) = default;
	ParticleObject& operator=(const ParticleObject&) = default;
	ParticleObject(ParticleObject&&) = default;
	ParticleObject& operator=(ParticleObject&&) = default;

private:
	void Initialize() override;
};