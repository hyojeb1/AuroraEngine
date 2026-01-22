///BOF FSMComponentEnemy.cpp
#include "stdafx.h"
#include "FSMComponentEnemy.h"
#include "GameObjectBase.h"
#include "InputManager.h"
#include "TimeManager.h"

REGISTER_TYPE(FSMComponentEnemy)

using namespace std;
using namespace DirectX;

std::string FSMComponentEnemy::StateToString(StateID state) const
{
	switch ((EGunState)state)
	{
	case EGunState::Idle:   return "Idle";
	case EGunState::Attack: return "Attack";
	case EGunState::Reload: return "Reload";
	default:                return "Unknown";
	}
}

FSMComponent::StateID FSMComponentEnemy::StringToState(const std::string& str) const
{
	if (str == "Idle")   return (StateID)EGunState::Idle;
	if (str == "Attack") return (StateID)EGunState::Attack;
	if (str == "Reload") return (StateID)EGunState::Reload;
	return (StateID)EGunState::Idle;
}

void FSMComponentEnemy::OnEnterState(StateID state)
{
	switch ((EGunState)state)
	{
	case EGunState::Idle:
		timer_ = 0.0f;
#ifdef _DEBUG
		cout << "Gun State: IDLE" << endl;
#endif
		break;

	case EGunState::Attack:
#ifdef _DEBUG
		cout << "Gun State: ATTACK START" << endl;
#endif
		timer_ = 0.0f;
		origin_rotation_ = GetOwner()->GetRotation();
		break;

	case EGunState::Reload:
		// Reload 로직
		break;
	}
}

void FSMComponentEnemy::OnUpdateState(StateID state)
{
	switch ((EGunState)state)
	{
	case EGunState::Idle:
		InputManager& input = InputManager::GetInstance();
		if (input.GetKeyDown(KeyCode::MouseLeft))
		{
			ChangeState((StateID)EGunState::Attack);
		}
		if (input.GetKeyDown(KeyCode::R))
		{
			ChangeState((StateID)EGunState::Attack);
		}
		break;

	case EGunState::Attack:
	{
		const float dt = TimeManager::GetInstance().GetDeltaTime();
		timer_ += dt;
		constexpr float kRecoilAngle = -90.0f;
		constexpr float kRecoilDuration = 0.12f;
		const float half_duration = kRecoilDuration * 0.5f;

		// 반동 계산 로직
		const XMVECTOR recoil_rotation = XMVectorSet(
			XMVectorGetX(origin_rotation_) + kRecoilAngle,
			XMVectorGetY(origin_rotation_),
			XMVectorGetZ(origin_rotation_),
			0.0f
		);

		float t = 0.0f;
		if (timer_ <= half_duration)
		{
			t = timer_ / half_duration;
			GetOwner()->SetRotation(XMVectorLerp(origin_rotation_, recoil_rotation, t));
		}
		else
		{
			t = (timer_ - half_duration) / half_duration;
			GetOwner()->SetRotation(XMVectorLerp(recoil_rotation, origin_rotation_, t));
		}

		if (timer_ >= kRecoilDuration)
		{
			GetOwner()->SetRotation(origin_rotation_);
			ChangeState((StateID)EGunState::Idle);
		}
	}
	break;

	default:
		break;
	}
}

void FSMComponentEnemy::OnExitState(StateID state)
{
	switch ((EGunState)state)
	{
	case EGunState::Idle:
		break;
	case EGunState::Attack:
		break;
	}
}
///EOF FSMComponentEnemy.cpp