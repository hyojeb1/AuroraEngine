#pragma once



class GameManager : public Singleton<GameManager>
{
	friend class Singleton<GameManager>;
	
    enum class GameState
    {
        Title,
        EnterMainScene,
        
    };

    // 상태 변수
    int     currentScore = 0;
    int     multiplier = 0;         // 1, 2, 4, 8
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
    void ScoreReset();          // 씬 바뀔때?

    const int GetScore() { return currentScore; }
    void SetScore(int num) { currentScore = num; }

    const int GetMultiplier() { return multiplier; }
    void SetMultiplier(int num) { multiplier = num; }
};