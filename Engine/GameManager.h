#pragma once

//enum class GameState
//{
//	Boot
// 
//	Tutorial, 
// 
//	MainPlaying,
//	MainPaused,
// 
//	EnterBoss,
// 
//	MainClear,
//	MainFail,
//	ReturnTitle
//};

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


struct TutorialController
{
    bool CanMove = false;
    bool CanDash = false;
    bool CanRelaod = false;
    bool CanShot = false;
    bool CanSkill = false;
    bool CanAutoReload = false;
};

enum class TutorialStep
{
    WASD,
    Dash,
    Reload,
    Shoot,
    AutoReload,
    DeadEye
};

class GameManager : public Singleton<GameManager>
{
	friend class Singleton<GameManager>;
	
    bool m_Pause = false;

    //점수 관련 변수
	int     m_currentScore = 0;
	int     m_multiplier = 0;         // 1, 2, 4, 8
	int     m_killCountForNextLevel = 0;
	float   m_lastKillTime = 0.0f;
	bool    m_isCombatStarted = false;
	float   m_decayTimer = 0.0f;

    void TempPrint();   // UI에 넣기 전에 저장할 위치
public:
    void Initailize();
    void Update();              // SceneManager.cpp - Run()
    void ScoreUpdate();
    void AddKill();             // Enemy.cpp - Die()
    void OnPlayerHit();         // Player.cpp - 뭐임
    void OnRhythmMiss();        // 리듬 미스 시 호출 (스택 초기화)
    void ScoreReset();          // 씬 바뀔때?

    const int GetScore() { return m_currentScore; }
    void SetScore(int num) { m_currentScore = num; }

    const int GetMultiplier() { return m_multiplier; }
    void SetMultiplier(int num) { m_multiplier = num; }
};