///BOF FSMComponentEnemy.cpp
#include "stdafx.h"
#include "FSMComponentEnemy.h"
#include "GameObjectBase.h"
#include "SkinnedModelComponent.h"
#include "TimeManager.h"

REGISTER_TYPE(FSMComponentEnemy)

using namespace std;
using namespace DirectX;


void FSMComponentEnemy::Initialize()
{
	FSMComponent::Initialize();
	model_ = GetOwner()->GetComponent<SkinnedModelComponent>();
}

std::string FSMComponentEnemy::StateToString(StateID state) const
{
	switch (state)
	{
	case EIdle:   return "Idle";
	case ERun:    return "Run";
	case EAttack: return "Attack";
	case EDead:   return "Dead";
	default:     return "Unknown";
	}
}

FSMComponent::StateID FSMComponentEnemy::StringToState(const std::string& str) const
{
	if (str == "Idle")   return EIdle;
	if (str == "Run")    return ERun;
	if (str == "Attack") return EAttack;
	if (str == "Dead")   return EDead;
	return EIdle;
}

void FSMComponentEnemy::OnEnterState(StateID state)
{
	if (!model_) return;

	switch (state)
	{
	case EIdle:
		model_->GetAnimator()->PlayAnimation("rig|rigAction", true);
		break;
	case EDead:
		death_timer_ = 0.0f;
		model_->GetAnimator()->PlayAnimation("rig|PlaneAction", false);
		break;
	case ERun:
	case EAttack:
		break;
	}
}

void FSMComponentEnemy::OnUpdateState(StateID state)
{
	float dt = TimeManager::GetInstance().GetDeltaTime();

	switch (state)
	{
	case EDead:
	{
		death_timer_ += dt;

		// 1.5초 후부터 0.5초간 서서히 투명해지기 (총 2초)
		// Enemy.h의 m_deathDuration(2.0f)과 타이밍을 맞춰야 함
		constexpr float kFadeStartTime = 1.5f;
		constexpr float kFadeDuration = 0.5f;

		if (death_timer_ >= kFadeStartTime)
		{
			float alpha = 1.0f - ((death_timer_ - kFadeStartTime) / kFadeDuration);
			if (alpha < 0.0f) alpha = 0.0f;

			// SkinnedModelComponent에 알파 조절 함수가 있다고 가정
			if (model_)
			{
				// model_->SetAlpha(alpha); 
				// 또는 Color 전체 변경:
				// model_->SetColor({1.0f, 1.0f, 1.0f, alpha}); 
			}
		}
	}
	break;
	}
}

void FSMComponentEnemy::OnExitState(StateID state)
{

}

void FSMComponentEnemy::RenderImGui()
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


///EOF FSMComponentEnemy.cpp