///BOF FSMComponentEnemy.h
///얜 판정을 하는 애가 아니야. 
///연출에 한정하는 애임
#pragma once
#include "FSMComponent.h"
#include <DirectXMath.h>

class SkinnedModelComponent;

class FSMComponentEnemy : public FSMComponent
{
public:
	enum EState
	{
		EIdle,
		ERun,     // Chase 상태일 때 재생
		EAttack,  // PreAttack 시점에 재생 시작
		EDead,
		ECount
	};

public:
	FSMComponentEnemy() = default;
	~FSMComponentEnemy() override = default;

	std::string StateToString(StateID state) const override;
	StateID StringToState(const std::string& str) const override;

	void SetModelComponent(SkinnedModelComponent* model) { model_ = model; }

protected:
	void OnEnterState(StateID state) override;
	void OnUpdateState(StateID state) override;
	void OnExitState(StateID state) override;

	void Initialize() override;
	#ifdef _DEBUG
	void RenderImGui() override;
	#endif

private:
	SkinnedModelComponent* model_ = nullptr;
	float death_timer_ = 0.0f;
};
///EOF FSMComponentEnemy.h