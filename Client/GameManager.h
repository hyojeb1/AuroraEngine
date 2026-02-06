#pragma once
#include "Player.h"

enum class EScene
{
    
	Title,
    Main,
	Result,
};

enum class EMainState
{
    Tutorial,
    Stage1,
    Stage2,
    StageBoss
};

enum class ETutorialStep
{
	WASD,
	Dash,
	Reload,
	Shoot,
	AutoReload,
	DeadEye,
    End
};


class GameManager : public Singleton<GameManager>
{
	friend class Singleton<GameManager>;
	

///GameFlow
    Player* m_Player = nullptr;

    bool m_Pause = false;

    EScene m_CurrentScene = EScene::Title;
    EMainState m_MainState = EMainState::Tutorial;
    ETutorialStep m_TutorialStep = ETutorialStep::WASD;
///GameFlowEnd

///SCORE
    //점수 관련 변수
	int     m_currentScore = 0;
	int     m_multiplier = 0;         // 1, 2, 4, 8
	int     m_killCountForNextLevel = 0;
	float   m_lastKillTime = 0.0f;
	bool    m_isCombatStarted = false;
	float   m_decayTimer = 0.0f;

    void TempPrint();   // UI에 넣기 전에 저장할 위치
///SCORE END


public:
    void Initialize();
    void Update();
    void Finalize();

    Player* GetPlayerPtr();
///GameFlow

    void OnSceneEnter(EScene type);
    void OnSceneUpdate();
    void OnSceneExit(EScene type);


    void MainSceneControl();
    void TutorialControl();



///GameFlowEND

///SCORE
    void ScoreUpdate();
    void AddKill();             // Enemy.cpp - Die()
    void OnPlayerHit();         // Player.cpp - 뭐임
    void OnRhythmMiss();        // 리듬 미스 시 호출 (스택 초기화)
    void ScoreReset();          // 씬 바뀔때?

    const int GetScore() { return m_currentScore; }
    void SetScore(int num) { m_currentScore = num; }

    const int GetMultiplier() { return m_multiplier; }
    void SetMultiplier(int num) { m_multiplier = num; }
///SCORE END
};
