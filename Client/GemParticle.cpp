#include "stdafx.h"
#include "GemParticle.h"

#include "TimeManager.h"

REGISTER_TYPE(GemParticle)

void GemParticle::Initialize()
{
	SetIgnoreParentTransform(true);
}