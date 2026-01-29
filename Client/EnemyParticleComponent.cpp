#include "stdafx.h"
#include "EnemyParticleComponent.h"

void EnemyParticleComponent::Initialize()
{
	ParticleComponent::Initialize();

	m_particleAmount = 50;
	m_particleTotalTime = 2.0f;
	uv_buffer_data_.imageScale = 1.0f;
	uv_buffer_data_.spreadRadius = 0.0f;
	uv_buffer_data_.spreadDistance = 5.0f;
}