///BOF FSMComponent.h
#pragma once
#include "ComponentBase.h"

enum class EState
{
	Idle,
	Move,
	Attack,
	Count
};

class FSMComponent : public ComponentBase
{
	using ActionFunc = std::function<void(float)>;
	using EventFunc = std::function<void()>;

private:
	EState current_state_ = EState::Idle;
	EState start_state_ = EState::Idle;

	std::unordered_map<EState, EventFunc>  enter_actions_ = { {EState::Idle, []() {}} };
	std::unordered_map<EState, ActionFunc> update_actions_ = { {EState::Idle, [](float) {}} };
	std::unordered_map<EState, EventFunc>  exit_actions_   = { {EState::Idle, []() {}} };

public:
	void SetStateLogics(EState state, EventFunc on_enter, ActionFunc on_update, EventFunc on_exit);
	void ChangeState(EState next_state);

	std::string StateToString(EState state) const;
	EState StringToState(const std::string& str) const;

	EState GetCurrentState() const { return current_state_; };

	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return true; }
	bool NeedsRender() const override { return false; }

	FSMComponent() = default;
	~FSMComponent() override = default;
	FSMComponent(const FSMComponent&) = default;
	FSMComponent& operator=(const FSMComponent&) = default;
	FSMComponent(FSMComponent&&) = default;
	FSMComponent& operator=(FSMComponent&&) = default;
protected:
	void Update() override;
	void Initialize() override;
	void RenderImGui() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

};


///EOF FSMComponent.h