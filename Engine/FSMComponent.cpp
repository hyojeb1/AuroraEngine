///BOF FSMComponent.cpp
#include "stdafx.h"
#include "FSMComponent.h"


//REGISTER_TYPE(FSMComponent)

using namespace std;
using namespace nlohmann;

void FSMComponent::ChangeState(StateID next_state)
{
	if (current_state_ == next_state)
		return;

	OnExitState(current_state_);
	current_state_ = next_state;
	OnEnterState(current_state_);
}

void FSMComponent::Initialize()
{
	current_state_ = start_state_;
	OnEnterState(current_state_);
}

void FSMComponent::Update()
{
	OnUpdateState(current_state_);
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