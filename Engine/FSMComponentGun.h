///BOF FSMComponentGun.h
#pragma once
#include "FSMComponent.h"
#include <DirectXMath.h>

class FSMComponentGun : public FSMComponent
{
public:
	FSMComponentGun() = default;
	~FSMComponentGun() override = default;

protected:
	void OnEnterState(EState state) override;
	void OnUpdateState(EState state, float delta_time) override;
	void OnExitState(EState state) override;

private:
	float recoil_timer_ = 0.0f;
	DirectX::XMVECTOR origin_rotation_{0,0,0,0};
};
///EOF FSMComponentGun.h