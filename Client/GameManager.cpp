#include "stdafx.h"
#include "GameManager.h"
#include "TimeManager.h"
#include "SceneManager.h"
#include "SceneBase.h"

#include "Shared/Config/Option.h"

void GameManager::Initialize()
{
}

void GameManager::Update()
{
	
}

void GameManager::Finalize()
{

}


void GameManager::OnSceneEnter(EScene type)
{
	switch (type)
	{
	case EScene::Title:
						ScoreReset();


		break;

	case EScene::Main:
						GetPlayerPtr();
		break;

	case EScene::Result:
		break;
	}
}

void GameManager::OnSceneUpdate()
{
	switch (m_CurrentScene)
	{
	case EScene::Title:
		break;
	case EScene::Main:
						MainSceneControl();
		break;
	case EScene::Result:
		break;
	}
}

void GameManager::OnSceneExit(EScene type)
{
	switch (type)
	{
	case EScene::Title:
		break;
	case EScene::Main:
		break;
	case EScene::Result:
		break;
	}
}

void GameManager::MainSceneControl()
{
	switch (m_MainState)
	{
	case EMainState::Tutorial:		TutorialControl();						break;
	case EMainState::Stage1:												break;
	case EMainState::Stage2:												break;
	case EMainState::StageBoss:												break;
	}
}

void GameManager::TutorialControl()
{
    auto& p = m_Player;

	p->SetAction(Action::All, false);

    switch (m_TutorialStep)
    {
	case ETutorialStep::WASD:			p->SetAction(Action::Move, true);			break;

    case ETutorialStep::Dash:			p->SetAction(Action::Move, true);
										p->SetAction(Action::Dash, true);			break;

    case ETutorialStep::Reload:			p->SetAction(Action::Reload, true);			break;

    case ETutorialStep::Shoot:			p->SetAction(Action::Shoot, true);			break;

	case ETutorialStep::AutoReload:		p->SetAction(Action::Shoot, true);			break;
    									p->SetAction(Action::AutoReload, true);

	case ETutorialStep::DeadEye:		p->SetAction(Action::DeadEye, true);		break;

	case ETutorialStep::End:			p->SetAction(Action::All, true);			break;
    }
}

Player* GameManager::GetPlayerPtr()
{
	Player* temp = dynamic_cast<Player*>(SceneManager::GetInstance().GetCurrentScene()->GetRootGameObject("Player"));
	return temp;
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
