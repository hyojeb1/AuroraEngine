#pragma once



class ScoreManager : public Singleton<ScoreManager>
{
	friend class Singleton<ScoreManager>;
	
    // 상태 변수
    int     currentScore = 0;
    int     multiplier = 1;         // 1, 2, 4, 8
    int     killCountForNextLevel = 0;
    float   lastKillTime = 0.0f;
    bool    isCombatStarted = false;
    float   decayTimer = 0.0f;

    void TempPrint(float dt);   // UI에 넣기 전에 저장할 위치
public:
    void Update(float dt);      // SceneManager.cpp - Run()
    void AddKill();             // Enemy.cpp - Die()
    void OnPlayerHit();         // Player.cpp - 뭐임
    void OnRhythmMiss();        // 리듬 미스 시 호출 (스택 초기화)
    void Reset();               // 씬 바뀔때?

    const int GetScore() { return currentScore; }
    const int GetMultiplier() { return multiplier; }
};