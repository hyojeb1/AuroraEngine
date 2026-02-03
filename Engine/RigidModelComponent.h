#pragma once
#include "ModelComponent.h"

class RigidModelComponent : public ModelComponent
{
private:
	std::shared_ptr<class Animator> animator_ = nullptr;
	
	struct BoneBuffer m_boneBufferData = {};
	com_ptr<ID3D11Buffer> m_boneConstantBuffer = nullptr; 

public:
	RigidModelComponent();
	virtual ~RigidModelComponent() override = default;
	RigidModelComponent(const RigidModelComponent&) = default;
	RigidModelComponent& operator=(const RigidModelComponent&) = default;
	RigidModelComponent(RigidModelComponent&&) = default;
	RigidModelComponent& operator=(RigidModelComponent&&) = default;

	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return true; }
	bool NeedsRender() const override { return true; }

	std::shared_ptr<Animator>& GetAnimator() { return animator_; }
private:
	void Initialize() override;
	void Update() override;
	void Render() override;
	#ifdef _DEBUG
	void RenderImGui() override;
	#endif
	void Finalize() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	void CreateShaders() override;
};