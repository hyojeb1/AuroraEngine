#include "stdafx.h"
#include "GemParticle.h"

#include "TimeManager.h"

REGISTER_TYPE(GemParticle)

void GemParticle::Initialize()
{
	SetIgnoreParentTransform(true);
}

void GemParticle::Update()
{
	m_lifeTime += TimeManager::GetInstance().GetDeltaTime();
	if (m_lifeTime >= m_maxLifeTime) SetAlive(false);
}