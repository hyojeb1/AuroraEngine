#include "stdafx.h"
#include "ParticleObject.h"

#include "TimeManager.h"
#include "ParticleComponent.h"

REGISTER_TYPE(ParticleObject)

void ParticleObject::Initialize()
{
	SetIgnoreParentTransform(true);
}