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
public:
	void ChangeState(EState next_state);
	EState GetCurrentState() const { return current_state_; };

	std::string StateToString(EState state) const;
	EState StringToState(const std::string& str) const;

	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return true; }
	bool NeedsRender() const override { return false; }

	FSMComponent() = default;
	virtual ~FSMComponent() override = default;
	FSMComponent(const FSMComponent&) = default;
	FSMComponent& operator=(const FSMComponent&) = default;
	FSMComponent(FSMComponent&&) = default;
	FSMComponent& operator=(FSMComponent&&) = default;

protected:
	virtual void OnEnterState(EState state) = 0;
	virtual void OnUpdateState(EState state, float delta_time) = 0;
	virtual void OnExitState(EState state) = 0;

	void Update() override;
	void Initialize() override;
	void RenderImGui() override;
	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

private:
	EState current_state_ = EState::Idle;
	EState start_state_ = EState::Idle;

};


///EOF FSMComponent.h