#include "stdafx.h"
#include "GameManager.h"
#include "TimeManager.h"

#include "Shared/Config/Option.h"

void GameManager::Initialize()
{
	ScoreReset();
	m_multiplier = 1;
}

void GameManager::Update()
{
	switch (m_currnetTutorialSetp)
	{
	case TutorialStep::WASD:
		m_tutorialController.CanMove = true;
		break;
	case TutorialStep::Dash:
		m_tutorialController.CanDash = true;
		break;
	case TutorialStep::Reload:
		m_tutorialController.CanRelaod = true;
		break;
	case TutorialStep::Shoot:
		m_tutorialController.CanShoot = true;
		break;
	case TutorialStep::AutoReload:
		m_tutorialController.CanAutoReload = true;
		break;
	case TutorialStep::DeadEye:
		m_tutorialController.CanSkill = true;
		break;
	}
}

void GameManager::ScoreUpdate()
{
	float delta = TimeManager::GetInstance().GetDeltaTime();

	if (!m_isCombatStarted)
		return;

	m_decayTimer += delta;
	while (m_decayTimer >= Config::ScoreDecayIntervalSec)
	{
		m_decayTimer -= Config::ScoreDecayIntervalSec;

		if (m_currentScore > 0)
		{
			m_currentScore -= Config::ScoreDecayAmount;
			if (m_currentScore < 0) m_currentScore = 0;
		}
		else
		{
			m_currentScore = 0;
			break;
		}
	}
}

void GameManager::ScoreReset()
{
	m_currentScore = 0;
	m_multiplier = 0;
	m_killCountForNextLevel = 0;
	m_lastKillTime = 0.0f;
	m_isCombatStarted = false;
	m_decayTimer = 0.0f;
}

void GameManager::TempPrint() 
{
	float dt = TimeManager::GetInstance().GetDeltaTime();

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

void GameManager::AddKill()
{
	const float now = TimeManager::GetInstance().GetTotalTime();

	int bonus = 0;
	if (m_isCombatStarted)
	{
		if (now - m_lastKillTime <= Config::ChainKillWindowSec)
			bonus = Config::ChainKillBonus;
	}
	else
	{
		m_isCombatStarted = true;
	}

	m_lastKillTime = now;

	const int gained = (Config::BaseScore + bonus) * m_multiplier;
	m_currentScore += gained;

	if (m_multiplier < Config::MaxMultiplier) {
		++m_killCountForNextLevel;
		if (m_killCountForNextLevel >= Config::KillsPerLevel) {
			if (m_multiplier == 1) m_multiplier = 2;
			else if (m_multiplier == 2) m_multiplier = 4;
			else if (m_multiplier == 4) m_multiplier = 8;
			m_killCountForNextLevel = 0;
		}
	}
}

void GameManager::OnPlayerHit()
{
	if (m_multiplier == 8) m_multiplier = 4;
	else if (m_multiplier == 4) m_multiplier = 2;
	else if (m_multiplier == 2) m_multiplier = 1;

	m_killCountForNextLevel = 0;
}

void GameManager::OnRhythmMiss()
{
	m_killCountForNextLevel = 0;
}
