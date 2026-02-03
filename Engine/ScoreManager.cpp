#include "stdafx.h"
#include "ScoreManager.h"
#include "TimeManager.h"

namespace
{
	constexpr int kBaseScore = 100;
	constexpr int kChainKillBonus = 50; // TODO: confirm desired bonus value
	constexpr float kChainKillWindowSec = 2.0f;
	constexpr int kKillsPerLevel = 10;
	constexpr int kMaxMultiplier = 8;
	constexpr float kScoreDecayIntervalSec = 1.0f;
	constexpr int kScoreDecayAmount = 1;
}

void ScoreManager::AddKill()
{
	const float now = TimeManager::GetInstance().GetTotalTime();

	int bonus = 0;
	if (isCombatStarted)
	{
		if (now - lastKillTime <= kChainKillWindowSec)
			bonus = kChainKillBonus;
	}
	else
	{
		isCombatStarted = true;
	}

	lastKillTime = now;

	const int gained = (kBaseScore + bonus) * multiplier;
	currentScore += gained;

	if (multiplier < kMaxMultiplier)
	{
		++killCountForNextLevel;
		if (killCountForNextLevel >= kKillsPerLevel)
		{
			if (multiplier == 1) multiplier = 2;
			else if (multiplier == 2) multiplier = 4;
			else if (multiplier == 4) multiplier = 8;
			killCountForNextLevel = 0;
		}
	}
}

void ScoreManager::OnPlayerHit()
{
	if (multiplier == 8) multiplier = 4;
	else if (multiplier == 4) multiplier = 2;
	else if (multiplier == 2) multiplier = 1;

	killCountForNextLevel = 0;
}

void ScoreManager::OnRhythmMiss()
{
	killCountForNextLevel = 0;
}

void ScoreManager::Update(float dt)
{
	if (!isCombatStarted)
		return;

	decayTimer += dt;
	while (decayTimer >= kScoreDecayIntervalSec)
	{
		decayTimer -= kScoreDecayIntervalSec;

		if (currentScore > 0)
		{
			currentScore -= kScoreDecayAmount;
			if (currentScore < 0) currentScore = 0;
		}
		else
		{
			currentScore = 0;
			break;
		}
	}
}
