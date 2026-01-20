///GunObject.cpp의 시작
#include "stdafx.h"
#include "GunObject.h"

#include "ModelComponent.h"
#include "FSMComponent.h"
#include "TimeManager.h"


using namespace std;
using namespace DirectX;

REGISTER_TYPE(GunObject)

namespace
{
	class GunIdle : public IState
	{
	public:
		const std::string& GetName() const override
		{
			static const std::string kName = "Idle";
			return kName;
		}
	};

	class GunRecoil : public IState
	{
	public:
		const std::string& GetName() const override
		{
			static const std::string kName = "Recoil";
			return kName;
		}

		void Enter(FSMComponent& machine) override
		{
			elapsed_time_ = 0.0f;
			if (GameObjectBase* owner = machine.GetOwner())
			{
				start_rotation_ = owner->GetRotation();
			}

#ifdef _DEBUG
			cout << "Enter Gun Recoil" << endl;
#endif // _DEBUG

		}

		void Exit(FSMComponent& machine) override
		{
#ifdef _DEBUG
			cout << "Exit Gun Recoil" << endl;
#endif // _DEBUG
		}

		void Update(FSMComponent& machine) override
		{

			GameObjectBase* owner = machine.GetOwner();
			if (!owner)
			{
				return;
			}

			elapsed_time_ += TimeManager::GetInstance().GetDeltaTime();

			constexpr float kRecoilAngle = -90.0f;
			constexpr float kRecoilDuration = 0.12f;
			const float half_duration = kRecoilDuration * 0.5f;

			const XMVECTOR recoil_rotation = XMVectorSet(
				XMVectorGetX(start_rotation_) + kRecoilAngle,
				XMVectorGetY(start_rotation_),
				XMVectorGetZ(start_rotation_),
				0.0f
			);

			float t = 0.0f;
			if (elapsed_time_ <= half_duration)
			{
				t = elapsed_time_ / half_duration;
				owner->SetRotation(XMVectorLerp(start_rotation_, recoil_rotation, t));
			}
			else
			{
				t = (elapsed_time_ - half_duration) / half_duration;
				owner->SetRotation(XMVectorLerp(recoil_rotation, start_rotation_, t));
			}

			if (elapsed_time_ >= kRecoilDuration)
			{
				owner->SetRotation(start_rotation_);
				machine.ChangeState("Idle");
			}
		}

	private:
		float elapsed_time_ = 0.0f;
		XMVECTOR start_rotation_ = XMVectorZero();
	};

}

void GunObject::Initialize()
{
	m_fsmComponent = GetComponent<FSMComponent>();
	if (!m_fsmComponent)
	{
		m_fsmComponent = CreateComponent<FSMComponent>();
	}
	if (m_fsmComponent)
	{
		m_fsmComponent->AddState<GunIdle>();
		m_fsmComponent->AddState<GunRecoil>();
		m_fsmComponent->ChangeState("Idle");
	}
}

void GunObject::Update()
{

}

void GunObject::Fire()
{
	if (!m_fsmComponent)
	{
		return;
	}

	if (m_fsmComponent->GetCurrentStateName() == "Recoil")
	{
		m_fsmComponent->ChangeState("Idle");
	}

	m_fsmComponent->ChangeState("Recoil");
}

///GunObject.cpp의 끝