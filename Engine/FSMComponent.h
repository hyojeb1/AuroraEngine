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
	virtual void Update(FSMComponent& machine) = 0;
	

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
	



	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return false; }
	bool NeedsRender() const override { return false; }

protected:
	void Initialize() override;
	void Update() override;
	void Render() override;
	void RenderImGui() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;
};

