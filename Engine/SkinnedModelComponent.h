#pragma once
#include "ModelComponent.h"

class SkinnedModelComponent : public ModelComponent
{
private:
	std::shared_ptr<class Animator> animator_ = nullptr;
	
	struct BoneBuffer m_boneBufferData = {};
	com_ptr<ID3D11Buffer> m_boneConstantBuffer = nullptr; 

public:
	SkinnedModelComponent();
	virtual ~SkinnedModelComponent() override = default;
	SkinnedModelComponent(const SkinnedModelComponent&) = default;
	SkinnedModelComponent& operator=(const SkinnedModelComponent&) = default;
	SkinnedModelComponent(SkinnedModelComponent&&) = default;
	SkinnedModelComponent& operator=(SkinnedModelComponent&&) = default;

	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return true; }
	bool NeedsRender() const override { return true; }

	std::shared_ptr<Animator>& GetAnimator() { return animator_; }
private:
	void Initialize() override;
	void Update() override;
	void Render() override;
	void RenderImGui() override;
	void Finalize() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	void CreateShaders() override;
};