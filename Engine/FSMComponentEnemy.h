///BOF FSMComponentEnemy.h
#pragma once
#include "FSMComponent.h"
#include <DirectXMath.h>

class SkinnedModelComponent;

class FSMComponentEnemy : public FSMComponent
{
public:
	FSMComponentEnemy() = default;
	~FSMComponentEnemy() override = default;

protected:
	void Initialize() override;
	void OnEnterState(EState state) override;
	void OnUpdateState(EState state, float delta_time) override;
	void OnExitState(EState state) override;

private:
	SkinnedModelComponent* skinned_model_ = nullptr;
};
///EOF FSMComponentEnemy.h