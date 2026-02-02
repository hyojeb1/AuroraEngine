#pragma once
#include "GameObjectBase.h"

class GemParticle : public GameObjectBase
{
public:
	GemParticle() = default;
	~GemParticle() override = default;
	GemParticle(const GemParticle&) = default;
	GemParticle& operator=(const GemParticle&) = default;
	GemParticle(GemParticle&&) = default;
	GemParticle& operator=(GemParticle&&) = default;

private:
	void Initialize() override;
};