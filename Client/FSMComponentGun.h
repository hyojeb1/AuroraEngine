///BOF FSMComponentGun.h
#pragma once
#include "FSMComponent.h"
#include <DirectXMath.h>

class FSMComponentGun : public FSMComponent
{
protected:
	enum EState
	{
		EIdle,   // 0
		EAttack, 
		EReload, 
		ECount
	};

public:
	FSMComponentGun() = default;
	~FSMComponentGun() override = default;

	std::string StateToString(StateID state) const override;
	StateID StringToState(const std::string& str) const override;
	
	void Fire();
	void Reload();

protected:
	void OnEnterState(StateID state) override;
	void OnUpdateState(StateID state) override;
	void OnExitState(StateID state) override;

	#ifdef _DEBUG
	void RenderImGui() override;
	#endif

private:

	float m_timer = 0.0f;
	DirectX::XMVECTOR m_originRotation{0,0,0,0};
};
///EOF FSMComponentGun.h