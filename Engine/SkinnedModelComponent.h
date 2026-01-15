/// SkinnedModelComponent.h의 시작
#pragma once
#include "ComponentBase.h"

extern class CameraComponent* g_mainCamera;

class SkinnedModelComponent : public ComponentBase
{
	com_ptr<ID3D11DeviceContext> m_deviceContext = nullptr; // 디바이스 컨텍스트

	const WorldNormalBuffer* m_worldNormalData = nullptr; // 월드, 월드 역행렬 상수 버퍼 데이터
	com_ptr<ID3D11Buffer> m_worldMatrixConstantBuffer = nullptr; // 월드, WVP 행렬 상수 버퍼

	std::string m_vsShaderName = "VSModelSkinAnim.hlsl"; // 기본 모델 정점 셰이더
	std::string m_psShaderName = "PSModel.hlsl"; // 기본 모델 픽셀 셰이더

	// 입력 요소 배열 // 위치, UV, 법선, 접선
	std::vector<InputElement> m_inputElements =
	{
		InputElement::Position,
		InputElement::UV,
		InputElement::Normal,
		InputElement::Bitangent,
		InputElement::Tangent,

		InputElement::Blendindex,
		InputElement::Blendweight

	};
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_vertexShaderAndInputLayout = {}; // 정점 셰이더 및 입력 레이아웃
	com_ptr<ID3D11PixelShader> m_pixelShader = nullptr; // 픽셀 셰이더

	std::string m_modelFileName = "Aurora6.fbx"; // 기본 모델 파일 이름

	const struct SkinnedModel* m_model = nullptr;

	MaterialFactorBuffer m_materialFactorData = {}; // 재질 상수 버퍼 데이터
	com_ptr<ID3D11Buffer> m_materialConstantBuffer = nullptr; // 재질 상수 버퍼
	
	BoneBuffer m_boneBufferData = {}; // 뼈 상수 버퍼 데이터
	com_ptr<ID3D11Buffer> m_boneConstantBuffer = nullptr; // 뼈 상수 버퍼

	BlendState m_blendState = BlendState::Opaque; // 기본 블렌드 상태
	RasterState m_rasterState = RasterState::Solid; // 기본 래스터 상태

	// 조인트 표시할려고... 만들어보자.
	std::string m_jointLineVSShaderName = "VSJointLine.hlsl";
	std::string m_jointLinePSShaderName = "PSJointLine.hlsl";
	std::vector<InputElement> m_jointLineInputElements = { InputElement::Position };
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_jointLineVertexShaderAndInputLayout = {};
	com_ptr<ID3D11PixelShader> m_jointLinePixelShader = nullptr;
	com_ptr<ID3D11Buffer> m_jointLineVertexBuffer = nullptr;
	size_t m_jointLineVertexCapacity = 0;

	bool m_bRenderSkeletonLines = true;
	bool m_bShowSkeletonTree = true;
	bool m_bShowJointOverlay = true;


public:
	SkinnedModelComponent() = default;
	~SkinnedModelComponent() override = default;
	SkinnedModelComponent(const SkinnedModelComponent&) = default;
	SkinnedModelComponent& operator=(const SkinnedModelComponent&) = default;
	SkinnedModelComponent(SkinnedModelComponent&&) = default;
	SkinnedModelComponent& operator=(SkinnedModelComponent&&) = default;

	const std::string& GetVertexShaderName() const { return m_vsShaderName; }
	void SetVertexShaderName(const std::string& vsShaderName) { m_vsShaderName = vsShaderName; }
	const std::string& GetPixelShaderName() const { return m_psShaderName; }
	void SetPixelShaderName(const std::string& psShaderName) { m_psShaderName = psShaderName; }

	const std::string& GetModelFileName() const { return m_modelFileName; }
	void SetModelFileName(const std::string& modelFileName) { m_modelFileName = modelFileName; }

	bool NeedsUpdate() const override { return false; }
	bool NeedsRender() const override { return true; }

private:
	void Initialize() override;
	void Render() override;
	void RenderImGui() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	// 셰이더 생성
	void CreateShaders();
};
/// SkinnedModelComponent.h의 끝