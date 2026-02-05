#include "stdafx.h"
#include "GameManager.h"
#include "TimeManager.h"

namespace
{
	constexpr int kBaseScore = 100;
	constexpr int kChainKillBonus = 50; // 임의 
	constexpr float kChainKillWindowSec = 2.0f;
	constexpr int kKillsPerLevel = 10;
	constexpr int kMaxMultiplier = 8;
	constexpr float kScoreDecayIntervalSec = 1.0f;
	constexpr int kScoreDecayAmount = 1;
}

void GameManager::AddKill()
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

	if (multiplier < kMaxMultiplier){
		++killCountForNextLevel;
		if (killCountForNextLevel >= kKillsPerLevel){
			if (multiplier == 1) multiplier = 2;
			else if (multiplier == 2) multiplier = 4;
			else if (multiplier == 4) multiplier = 8;
			killCountForNextLevel = 0;
		}
	}
}

void GameManager::OnPlayerHit()
{
	if (multiplier == 8) multiplier = 4;
	else if (multiplier == 4) multiplier = 2;
	else if (multiplier == 2) multiplier = 1;

	killCountForNextLevel = 0;
}

void GameManager::OnRhythmMiss()
{
	killCountForNextLevel = 0;
}

void GameManager::Update(float dt)
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
	
	TempPrint(dt);
}

void GameManager::ScoreReset()
{
	currentScore = 0;
	multiplier = 0;
	killCountForNextLevel = 0;
	lastKillTime = 0.0f;
	isCombatStarted = false;
	decayTimer = 0.0f;
}

void GameManager::TempPrint(float dt) 
{
	static float scorePrintTimer = 0.0f;
	scorePrintTimer += dt;
	if (scorePrintTimer >= 0.5f) {
		scorePrintTimer = 0.0f;
		std::cout << std::endl << "[Score] "
			<< GameManager::GetInstance().GetScore()
			<< " x"
			<< GameManager::GetInstance().GetMultiplier()
			<< std::endl;
	}
}