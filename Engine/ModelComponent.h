/// ModelComponent.h의 시작
#pragma once
#include "ComponentBase.h"
//#include "Resource.h"

class ModelComponent : public ComponentBase
{
protected:
	com_ptr<ID3D11DeviceContext> m_deviceContext = nullptr; // 디바이스 컨텍스트

	const WorldNormalBuffer* m_worldNormalData = nullptr; // 월드, 월드 역행렬 상수 버퍼 데이터

	std::string m_vsShaderName = "VSModel.hlsl"; // 기본 모델 정점 셰이더
	std::string m_psShaderName = "PSModel.hlsl"; // 기본 모델 픽셀 셰이더

	// 입력 요소 배열 // 위치, UV, 법선, 접선
	std::vector<InputElement> m_inputElements =
	{
		InputElement::Position,
		InputElement::UV,
		InputElement::Normal,
		InputElement::Bitangent,
		InputElement::Tangent
	};
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_vertexShaderAndInputLayout = {}; // 정점 셰이더 및 입력 레이아웃
	com_ptr<ID3D11PixelShader> m_pixelShader = nullptr; // 픽셀 셰이더

	std::string m_modelFileName = "box.fbx"; // 기본 모델 파일 이름

	const struct Model* m_model = nullptr;
	DirectX::BoundingBox m_boundingBox = {}; // 변환된 경계 상자

	MaterialFactorBuffer m_materialFactorData = {}; // 재질 상수 버퍼 데이터

	BlendState m_blendState = BlendState::Opaque; // 기본 블렌드 상태
	RasterState m_rasterState = RasterState::Solid; // 기본 래스터 상태

	#ifdef _DEBUG
	bool m_renderBoundingBox = true; // 경계 상자 렌더링 여부
	bool m_renderSubMeshBoundingBoxes = false; // 서브 메시 경계 상자 렌더링 여부
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_boundingBoxVertexShaderAndInputLayout = {}; // 경계 상자 정점 셰이더 및 입력 레이아웃
	com_ptr<ID3D11PixelShader> m_boundingBoxPixelShader = nullptr; // 경계 상자 픽셀 셰이더
	#endif

public:
	ModelComponent() = default;
	virtual ~ModelComponent() override = default;
	ModelComponent(const ModelComponent&) = default;
	ModelComponent& operator=(const ModelComponent&) = default;
	ModelComponent(ModelComponent&&) = default;
	ModelComponent& operator=(ModelComponent&&) = default;

	const std::string& GetVertexShaderName() const { return m_vsShaderName; }
	void SetVertexShaderName(const std::string& vsShaderName) { m_vsShaderName = vsShaderName; }
	const std::string& GetPixelShaderName() const { return m_psShaderName; }
	void SetPixelShaderName(const std::string& psShaderName) { m_psShaderName = psShaderName; }

	const std::string& GetModelFileName() const { return m_modelFileName; }
	void SetModelFileName(const std::string& modelFileName) { m_modelFileName = modelFileName; }

	void SetBlendState(BlendState blendState) { m_blendState = blendState; }
	void SetAlpha(const float& alpha) { m_materialFactorData.baseColorFactor.w = alpha; }

	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return false; }
	bool NeedsRender() const override { return true; }

protected:
	void Initialize() override;
	void Render() override;
	void RenderImGui() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	// 셰이더 생성
	virtual void CreateShaders();
};
/// ModelComponent.h의 끝