/// SkinnedModelComponent.h의 시작
#pragma once
#include "ModelComponent.h"

extern class CameraComponent* g_mainCamera;

class SkinnedModelComponent : public ModelComponent
{
private:

	std::shared_ptr<Animator> animator_ = nullptr;
	
	BoneBuffer m_boneBufferData = {}; // 뼈 상수 버퍼 데이터
	com_ptr<ID3D11Buffer> m_boneConstantBuffer = nullptr; // 뼈 상수 버퍼


	// 조인트 표시할려고... 만들어보자.
	std::string m_lineVSShaderName = "VSLine.hlsl";
	std::string m_linePSShaderName = "PSColor.hlsl";
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_lineVertexShaderAndInputLayout = {}; // 정점 셰이더 및 입력 레이아웃
	com_ptr<ID3D11PixelShader> m_linePixelShader = nullptr; // 픽셀 셰이더
	com_ptr<ID3D11Buffer> m_lineConstantBuffer = nullptr;

	bool m_bRenderSkeletonLines = false;
	bool m_bShowSkeletonTree = false;
	bool m_bShowJointOverlay = false;


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