///BOF FSMComponent.cpp
#include "stdafx.h"
#include "FSMComponent.h"
#include "TimeManager.h"

//REGISTER_TYPE(FSMComponent)

using namespace std;
using namespace nlohmann;

void FSMComponent::ChangeState(EState next_state)
{
	if (current_state_ == next_state)
		return;

	OnExitState(current_state_);

	current_state_ = next_state;

	OnEnterState(current_state_);
}

string FSMComponent::StateToString(EState state) const
{
	switch (state)
	{
	case EState::Idle:   return "Idle";
	case EState::Move:   return "Move";
	case EState::Attack: return "Attack";
	default:             return "None";
	}
}

EState FSMComponent::StringToState(const string& str) const
{
	if (str == "Idle")   return EState::Idle;
	if (str == "Move")   return EState::Move;
	if (str == "Attack") return EState::Attack;

	return EState::Idle;
}

void FSMComponent::Initialize()
{
	current_state_ = start_state_;
	OnEnterState(current_state_);
}

void FSMComponent::Update()
{
	float delta_time = TimeManager::GetInstance().GetDeltaTime();
	OnUpdateState(current_state_, delta_time);
}

void FSMComponent::RenderImGui()
{
	if (ImGui::TreeNode("FSM Component"))
	{
		string currentName = StateToString(current_state_);
		ImGui::Text("Current State: %s", currentName.c_str());

		if (ImGui::BeginCombo("Force State", currentName.c_str()))
		{
			for (int i = 0; i < (int)EState::Count; ++i)
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

json FSMComponent::Serialize()
{
	json data;
	data["current_state"] = StateToString(current_state_);
	data["start_state"] = StateToString(start_state_);
	return data;
}

void FSMComponent::Deserialize(const json& jsonData)
{
	if (jsonData.contains("current_state"))
	{
		current_state_ = StringToState(jsonData["current_state"].get<string>());
	}

	if (jsonData.contains("start_state"))
	{
		start_state_ = StringToState(jsonData["start_state"].get<string>());
	}
}

///EOF FSMComponent.cpp