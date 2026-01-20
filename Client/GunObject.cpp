///GunObject.cpp의 시작
#include "stdafx.h"
#include "GunObject.h"

#include "ModelComponent.h"
#include "FSMComponent.h"
#include "TimeManager.h"

REGISTER_TYPE(GunObject)

using namespace std;
using namespace DirectX;

void GunObject::Fire()
{
	auto fsm = GetComponent<FSMComponent>();
	if (fsm->GetCurrentState() != EState::Attack)
	{
		fsm->ChangeState(EState::Attack);
	}
}

void GunObject::Initialize()
{
	auto fsm = GetComponent<FSMComponent>();
	if (!fsm){ fsm = CreateComponent<FSMComponent>();}

	// --IDLE 상태 정의------------
	auto on_idle_enter = [this]()
		{
			recoil_timer_ = 0.0f;
		#ifdef _DEBUG
			std::cout << "Gun is now Idle." << std::endl;
		#endif
		};
	auto on_idle_update = [this](float dt)
		{

		};
	auto on_idle_exit = [this]()
		{
		#ifdef _DEBUG
			std::cout << "Gun is not Idle anymore." << std::endl;
		#endif
		};

	// --ATTACK 상태 정의------------
	auto on_attack_enter = [this]()
		{
		#ifdef _DEBUG
			std::cout << "Start Shooting!" << std::endl;
		#endif
			recoil_timer_ = 0.0f;
			origin_rotation_ = GetRotation();
		};
	auto on_attack_update = [this, fsm](float dt)
		{
			recoil_timer_ += dt;
			constexpr float kRecoilAngle = -90.0f;
			constexpr float kRecoilDuration = 0.12f;
			const float half_duration = kRecoilDuration * 0.5f;
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
				this->SetRotation(XMVectorLerp(origin_rotation_, recoil_rotation, t));
			}
			else
			{
				t = (recoil_timer_ - half_duration) / half_duration;
				this->SetRotation(XMVectorLerp(recoil_rotation, origin_rotation_, t));
			}

			if (recoil_timer_ >= kRecoilDuration)
			{
				this->SetRotation(origin_rotation_);
				fsm->ChangeState(EState::Idle);
			}
		};

	auto on_attack_exit = [this]()
		{
		#ifdef _DEBUG
			std::cout << "Stop Shooting." << std::endl;
		#endif
		};

	// --FSM 등록------------
	fsm->SetStateLogics(EState::Idle, on_idle_enter, on_idle_update, on_idle_exit);
	fsm->SetStateLogics(EState::Attack, on_attack_enter, on_attack_update, on_attack_exit);

}

void GunObject::Update()
{

}

///GunObject.cpp의 끝