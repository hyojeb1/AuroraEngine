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

public:
    void AddKill();             // 적 처치 시 호출
    void OnPlayerHit();         // 피격 시 호출 (배율 하락)
    void OnRhythmMiss();        // 리듬 미스 시 호출 (스택 초기화)
    void Update(float dt);      // 점수 차감 로직 처리

    int GetScore() { return currentScore; }
    int GetMultiplier() { return multiplier; }
};