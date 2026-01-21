#pragma once

class ListenerComponent : public ComponentBase
{
public:
	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return true; }
	bool NeedsRender() const override { return false; }

protected:
	void Initialize() override;
	void Update() override;
	void Render() override;
	void RenderImGui() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;
};

