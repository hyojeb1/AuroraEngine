///BOF FSMComponentEnemy.h
#pragma once
#include "FSMComponent.h"
#include <DirectXMath.h>

class FSMComponentEnemy : public FSMComponent
{
public:
	FSMComponentEnemy() = default;
	~FSMComponentEnemy() override = default;

protected:
	void OnEnterState(EState state) override;
	void OnUpdateState(EState state, float delta_time) override;
	void OnExitState(EState state) override;

private:
	float recoil_timer_ = 0.0f;
	DirectX::XMVECTOR origin_rotation_{0,0,0,0};
};
///EOF FSMComponentEnemy.h