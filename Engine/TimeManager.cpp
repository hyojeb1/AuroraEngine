#include "stdafx.h"
#include "TimeManager.h"
#include "SoundManager.h"

void TimeManager::Initialize()
{
	m_previousTime = timeGetTime();
}

void TimeManager::UpdateTime()
{
	m_currentTime = timeGetTime();

	constexpr float millisecondsToSeconds = 1.0f / 1000.0f;
	m_deltaTime = static_cast<float>(m_currentTime - m_previousTime) * millisecondsToSeconds * m_timeScale;
	m_deltaNoneScaleTime = static_cast<float>(m_currentTime - m_previousTime) * millisecondsToSeconds;
	m_totalTime += m_deltaTime;
	m_previousTime = m_currentTime;
}