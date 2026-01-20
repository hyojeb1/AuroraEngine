///BOF FSMComponent.cpp
#include "stdafx.h"
#include "FSMComponent.h"

REGISTER_TYPE(FSMComponent)

using namespace std;

void FSMComponent::Initialize()
{
	if (!initialize_state_name_.empty())
	{
		ChangeState(initialize_state_name_);
	}
}

void FSMComponent::Update()
{
	if (!current_state_ && !initialize_state_name_.empty())
	{
		ChangeState(initialize_state_name_);
	}

	if(current_state_)
	{
		current_state_->Update(*this);
	}
}

void FSMComponent::RenderImGui()
{
	array<char, 256> initialize_state_name_buffer = {};
	strcpy_s(initialize_state_name_buffer.data(), initialize_state_name_buffer.size(), initialize_state_name_.c_str());
	if (ImGui::InputText("Initialize State Name", initialize_state_name_buffer.data(), sizeof(initialize_state_name_buffer))) 
		initialize_state_name_ = initialize_state_name_buffer.data();
	
	const auto& kCurrentName = GetCurrentStateName();

	ImGui::Text("Current State Name: %s", GetCurrentStateName().c_str());

	ImGui::Separator();

	if (ImGui::BeginCombo("Change State", kCurrentName.c_str()))
	{
		for (const auto& [name, statePtr] : states_)
		{
			bool isSelected = (kCurrentName == name);
			if (ImGui::Selectable(name.c_str(), isSelected))
			{
				ChangeState(name);
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

nlohmann::json FSMComponent::Serialize()
{
	nlohmann::json json_data;
	json_data["initialState"] = initialize_state_name_;
	json_data["currentState"] = GetCurrentStateName();
	return json_data;
}

void FSMComponent::Deserialize(const nlohmann::json& json_data)
{
	if (json_data.contains("initialState"))
	{
		initialize_state_name_ = json_data["initialState"].get<std::string>();
	}

	if (json_data.contains("currentState"))
	{
		initialize_state_name_ = json_data["currentState"].get<std::string>();
	}

}


void FSMComponent::RegisterState(std::unique_ptr<IState> state)
{
	if (!state) return;

	const string state_name = state->GetName();
	if (state_name.empty()) return;

	states_[state_name] = move(state);
}

bool FSMComponent::ChangeState(const std::string& state_name)
{
	auto it = states_.find(state_name);
	if(it == states_.end()) { return false;}

	IState* next_state = it->second.get();
	if(next_state == current_state_) { return false; }
	
	if (current_state_) {
		current_state_->Exit(*this);
	}
	current_state_ = next_state;
	current_state_->Enter(*this);
	return true;
}

const std::string& FSMComponent::GetCurrentStateName() const
{
	static const std::string kEmpty = "";
	if (!current_state_)
	{
		return kEmpty;
	}

	return current_state_->GetName();
}
///EOF FSMComponent.cpp