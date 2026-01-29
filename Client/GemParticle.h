#pragma once
#include "GameObjectBase.h"

class GemParticle : public GameObjectBase
{
	float m_lifeTime = 0.0f;
	const float m_maxLifeTime = 3.0f;

public:
	GemParticle() = default;
	~GemParticle() override = default;
	GemParticle(const GemParticle&) = default;
	GemParticle& operator=(const GemParticle&) = default;
	GemParticle(GemParticle&&) = default;
	GemParticle& operator=(GemParticle&&) = default;

private:
	void Initialize() override;
	void Update() override;
};