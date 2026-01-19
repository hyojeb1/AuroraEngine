/// SkinnedModelComponent.h의 시작
#pragma once
#include "ModelComponent.h"

extern class CameraComponent* g_mainCamera;

class SkinnedModelComponent : public ModelComponent
{
private:

	std::shared_ptr<Animator> animator_ = nullptr;
	
	BoneBuffer m_boneBufferData = {};
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

private:
	void Initialize() override;
	void Update() override;
	void Render() override;
	void RenderImGui() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	// 셰이더 생성
	void CreateShaders();
};
/// SkinnedModelComponent.h의 끝