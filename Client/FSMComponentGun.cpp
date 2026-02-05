///BOF FSMComponentGun.cpp
#include "stdafx.h"
#include "FSMComponentGun.h"
#include "GameObjectBase.h"
#include "TimeManager.h"

REGISTER_TYPE(FSMComponentGun)

using namespace std;
using namespace DirectX;

std::string FSMComponentGun::StateToString(StateID state) const
{
	switch (state)
	{
	case EIdle:   return "Idle";
	case EAttack: return "Attack";
	case EReload: return "Reload";
	default:                
		return "Unknown";
	}
}

void FSMComponentGun::Fire()
{
	if (current_state_ == EReload)
		return;

	if (current_state_ == EAttack)
	{
		m_timer = 0.0f;
	}
	else
	{
		ChangeState(EAttack);
	}
}

void FSMComponentGun::Reload()
{
	if (current_state_ == EReload) return;

	ChangeState(EReload);
}

FSMComponent::StateID FSMComponentGun::StringToState(const std::string& str) const
{
	if (str == "Idle")   return EIdle;
	if (str == "Attack") return EAttack;
	if (str == "Reload") return EReload;
	return EIdle;
}

void FSMComponentGun::OnEnterState(StateID state)
{
	m_timer = 0.0f;
	switch (state)
	{
	case EIdle:
		break;

	case EAttack:
	case EReload:
		m_originRotation = GetOwner()->GetRotation();
		break;
	}
}

void FSMComponentGun::OnUpdateState(StateID state)
{
	const float dt = TimeManager::GetInstance().GetDeltaTime();

	switch (state)
	{
	case EIdle:
		break;

	case EAttack:
	{
		m_timer += dt;

		// 반동 설정 (Degree 단위)
		constexpr float kRecoilPitch = -15.0f; // 총구가 위로 들림 (-Pitch)
		constexpr float kRecoilDuration = 0.12f;
		const float half_duration = kRecoilDuration * 0.5f;

		XMVECTOR target_rotation = m_originRotation;
		// X축(Pitch)에 값을 더함 (GameObjectBase가 Degree를 쓴다고 가정)
		target_rotation.m128_f32[0] += kRecoilPitch;

		float t = 0.0f;
		if (m_timer <= half_duration)
		{
			// 갈 때 (Lerp)
			t = m_timer / half_duration;
			GetOwner()->SetRotation(XMVectorLerp(m_originRotation, target_rotation, t));
		}
		else
		{
			// 올 때 (Lerp)
			t = (m_timer - half_duration) / half_duration;
			GetOwner()->SetRotation(XMVectorLerp(target_rotation, m_originRotation, t));
		}

		if (m_timer >= kRecoilDuration)
		{
			GetOwner()->SetRotation(m_originRotation); // 정확히 원위치
			ChangeState(EIdle);
		}
	}
	break;
	case EReload:
	{
		m_timer += dt;

		// 한 바퀴 돌리기 설정 (360도 회전)
		constexpr float kReloadDuration = 0.5f; // 0.5초 동안
		constexpr float kSpinAngle = 360.0f;    // 360도 회전

		// 진행률 (0.0 ~ 1.0)
		float t = m_timer / kReloadDuration;

		if (t <= 1.0f)
		{
			// Z축(Roll)을 기준으로 빙글 돌리기 (권총 돌리기 연출)
			// m_originRotation 에서 Z값만 360도 증가시킨 벡터를 타겟으로 잡음

			// 단순 Lerp 방식:
			// XMVECTOR current_rot = m_originRotation;
			// current_rot.m128_f32[2] += (kSpinAngle * t); // Z축에 각도 누적

			// *주의*: GameObjectBase::SetRotation은 쿼터니언을 재생성하므로 
			// 360도가 넘어가도(e.g., 370도) 잘 처리되는지 확인 필요.
			// 보통 오일러각 Lerp는 360도 회전을 위해 값을 계속 증가시켜야 함.

			XMVECTOR next_rotation = m_originRotation;
			next_rotation.m128_f32[2] += (kSpinAngle * t); // Z축 회전 (Roll)

			GetOwner()->SetRotation(next_rotation);
		}
		else
		{
			// 끝났으면 원위치 및 Idle 복귀
			GetOwner()->SetRotation(m_originRotation);
			ChangeState(EIdle);
		}
	}
		break;
	}
}

void FSMComponentGun::OnExitState(StateID state)
{
	switch (state)
	{
	case EIdle:
		break;
	case EAttack:
		break;
	}
}

#ifdef _DEBUG
void FSMComponentGun::RenderImGui()
{
	if (ImGui::TreeNode("FSM Component Gun"))
	{
		string currentName = StateToString(current_state_);
		ImGui::Text("Current State: %s", currentName.c_str());

		if (ImGui::BeginCombo("Force State", currentName.c_str()))
		{
			for (int i = 0; i < ECount; ++i)
			{
				EState state = (EState)i;
				string stateName = StateToString(state);
				bool isSelected = (current_state_ == state);
				if (ImGui::Selectable(stateName.c_str(), isSelected))
				{
					ChangeState(state);
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::TreePop();
	}
}
#endif


///EOF FSMComponentGun.cpp