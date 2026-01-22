///BOF FSMComponentEnemy.h
#pragma once
#include "FSMComponent.h"
#include <DirectXMath.h>

class FSMComponentEnemy : public FSMComponent
{
protected:
	enum EGunState
	{
		Idle,   // 0
		Attack, // 1
		Reload, // 2
		Count
	};

public:
	FSMComponentEnemy() = default;
	~FSMComponentEnemy() override = default;

	std::string StateToString(StateID state) const override;
	StateID StringToState(const std::string& str) const override;

protected:
	void OnEnterState(StateID state) override;
	void OnUpdateState(StateID state) override;
	void OnExitState(StateID state) override;

private:

	float timer_ = 0.0f;
	DirectX::XMVECTOR origin_rotation_{ 0,0,0,0 };
};
///EOF FSMComponentEnemy.h