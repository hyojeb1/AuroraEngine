#include "stdafx.h"
#include "ParticleObject.h"

#include "TimeManager.h"

REGISTER_TYPE(ParticleObject)

void ParticleObject::Initialize()
{
	SetIgnoreParentTransform(true);
}

void ParticleObject::Update()
{
	m_lifetime -= TimeManager::GetInstance().GetDeltaTime();
	if (m_lifetime <= 0.0f) SetAlive(false);
}