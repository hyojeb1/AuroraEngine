#pragma once
#include "ComponentBase.h"

class EmitterComponent : public ComponentBase
{
public:
	std::string m_sourceName;

	void SFX_Shot(std::string filename);

	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return false; }
	bool NeedsRender() const override { return false; }

protected:
	void Initialize() override;
	void Update() override;
	void Render() override;
	#ifdef _DEBUG
	void RenderImGui() override;
	#endif

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;
};

