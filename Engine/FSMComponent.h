///BOF FSMComponent.h
#pragma once
#include "ComponentBase.h"


class FSMComponent : public ComponentBase
{
protected:
	using StateID = int;

public:
	void ChangeState(StateID next_state);
	StateID GetCurrentState() const { return current_state_; };

	virtual std::string StateToString(StateID state) const = 0;
	virtual StateID StringToState(const std::string& str) const = 0;

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
	virtual void OnEnterState(StateID state) = 0;
	virtual void OnUpdateState(StateID state) = 0;
	virtual void OnExitState(StateID state) = 0;

	void Update() override;
	void Initialize() override;
	void RenderImGui() override;
	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	StateID current_state_ = 0; 
	StateID start_state_ = 0;
};


///EOF FSMComponent.h