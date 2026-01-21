///BOF FSMComponentGun.cpp
#include "stdafx.h"
#include "FSMComponentGun.h"
#include "GameObjectBase.h"

REGISTER_TYPE(FSMComponentGun)

using namespace std;
using namespace DirectX;

void FSMComponentGun::OnEnterState(EState state)
{
	switch (state)
	{
	case EState::Idle:
		recoil_timer_ = 0.0f;
#ifdef _DEBUG
		cout << "Gun is now Idle." << endl;
#endif
		break;

	case EState::Attack:
#ifdef _DEBUG
		cout << "Start    !" << endl;
#endif
		recoil_timer_ = 0.0f;
		origin_rotation_ = GetOwner()->GetRotation();
		break;

	default:
		break;
	}
}

void FSMComponentGun::OnUpdateState(EState state, float dt)
{
	switch (state)
	{
	case EState::Idle:
		break;

	case EState::Attack:
	{
		recoil_timer_ += dt;
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
		if (recoil_timer_ <= half_duration)
		{
			t = recoil_timer_ / half_duration;
			GetOwner()->SetRotation(XMVectorLerp(origin_rotation_, recoil_rotation, t));
		}
		else
		{
			t = (recoil_timer_ - half_duration) / half_duration;
			GetOwner()->SetRotation(XMVectorLerp(recoil_rotation, origin_rotation_, t));
		}

		if (recoil_timer_ >= kRecoilDuration)
		{
			GetOwner()->SetRotation(origin_rotation_);
			ChangeState(EState::Idle);
		}
	}
	break;

	default:
		break;
	}
}

void FSMComponentGun::OnExitState(EState state)
{
	switch (state)
	{
	case EState::Idle:
#ifdef _DEBUG
		cout << "Gun is not Idle anymore." << endl;
#endif
		break;

	case EState::Attack:
#ifdef _DEBUG
		cout << "Stop Shooting." << endl;
#endif
		break;

	default:
		break;
	}
}
///EOF FSMComponentGun.cpp