///BOF FSMComponentEnemy.h
///얜 판정을 하는 애가 아니야. 
///연출에 한정하는 애임
#pragma once
#include "FSMComponent.h"
#include <DirectXMath.h>

class FSMComponentEnemy : public FSMComponent
{
public:
	enum EState
	{
		EIdle,
		EChase,
		EAttack,
		EDead,
		ECount
	};

public:
	FSMComponentEnemy() = default;
	~FSMComponentEnemy() override = default;

	std::string StateToString(StateID state) const override;
	StateID StringToState(const std::string& str) const override;

	void SetModelComponent(class SkinnedModelComponent* model) { model_ = model; }

protected:
	void OnEnterState(StateID state) override;
	void OnUpdateState(StateID state) override;
	void OnExitState(StateID state) override;

	void Initialize() override;
	#ifdef _DEBUG
	void RenderImGui() override;
	#endif

private:
	class SkinnedModelComponent* model_ = nullptr;
	class Enemy* owner_enemy_ = nullptr;	
	class Player* player_ = nullptr;

	float death_timer_ = 0.0f;

	float attack_timer_ = 0.0f;
	bool  attack_has_hit_ = false; //?



	const float kAttackRange = 3.2f;
	const float kAttackAnticipation = 0.35f; // 선딜
	const float kAttackTotalTime = 0.9f;     // 전체 모션 시간
	const int   kDamage = 1;

	const float kFadeStartTime = 0.5f;
	const float kFadeDuration = 1.5f;

};


///EOF FSMComponentEnemy.h