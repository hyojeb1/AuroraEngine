///BOF FSMComponent.h
#pragma once
#include "ComponentBase.h"

class FSMComponent;

class IState
{
public:
	virtual ~IState() = default;
	virtual const std::string& GetName() const = 0;
	virtual void Enter(FSMComponent& machine) {}
	virtual void Exit(FSMComponent& machine) {}
	virtual void Update(FSMComponent& machine) {};
	

protected:
	IState() = default;
	IState(const IState&) = default;
	IState& operator=(const IState&) = default;
	IState(IState&&) = default;
	IState& operator=(IState&&) = default;
};

class FSMComponent : public ComponentBase
{
public:
	void RegisterState(std::unique_ptr<IState> state);
	bool ChangeState(const std::string& state_name);
	const std::string& GetCurrentStateName() const;

	template<typename T> requires std::derived_from<T, IState>
	T* AddState();

	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return true; }
	bool NeedsRender() const override { return false; }

protected:
	void Update() override;
	void Initialize() override;
	void RenderImGui() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

private:
	std::unordered_map<std::string, std::unique_ptr<IState>> states_ = {};
	IState* current_state_ = nullptr;
	std::string initialize_state_name_ = {};

public:
	FSMComponent() = default;
	~FSMComponent() override = default;
	FSMComponent(const FSMComponent&) = default;
	FSMComponent& operator=(const FSMComponent&) = default;
	FSMComponent(FSMComponent&&) = default;
	FSMComponent& operator=(FSMComponent&&) = default;
};

template<typename T> requires std::derived_from<T, IState>
inline T* FSMComponent::AddState()
{
	std::unique_ptr<T> state = std::make_unique<T>();
	T* state_ptr = state.get();
	RegisterState(std::move(state));
	return state_ptr;
}
///EOF FSMComponent.h