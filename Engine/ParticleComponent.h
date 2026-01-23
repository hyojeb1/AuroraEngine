/// ParticleComponent.h의 시작
///
/// 바운딩 박스를 이용한 컬링은 주석으로 남겨 놓겠습니다. 
/// 성능을 위해서 중점 컬링을 합니다.
/// 하지만 만약 큰 파티클의 경우 바운딩 박스를 이용하거나 구면컬링을 준비해야 자연스러울 것입니다.
/// 
#pragma once
#include "ComponentBase.h"
//#include "Resource.h"

class ParticleComponent : public ComponentBase
{
protected:
	struct VertexPosUV{
		DirectX::XMFLOAT4 position = {};
		DirectX::XMFLOAT2 UV = {};
	};

	com_ptr<ID3D11Buffer> m_vertexBuffer = nullptr;

	com_ptr<ID3D11DeviceContext> m_deviceContext = nullptr; // 디바이스 컨텍스트
	const WorldNormalBuffer* m_worldNormalData = nullptr; // 월드, 월드 역행렬 상수 버퍼 데이터

	std::string m_vsShaderName = "VSParticle.hlsl"; // 기본 Particle 정점 셰이더
	std::string m_psShaderName = "PSParticle.hlsl"; // 기본 Particle 픽셀 셰이더

	// 입력 요소 배열 // 위치, UV
	std::vector<InputElement> m_inputElements =
	{
		InputElement::Position,
		InputElement::UV
	};

	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_vertexShaderAndInputLayout = {}; // 정점 셰이더 및 입력 레이아웃
	com_ptr<ID3D11PixelShader> m_pixelShader = nullptr; // 픽셀 셰이더


	std::array<VertexPosUV, 4> quad_ = {
	VertexPosUV{ {-0.5f,  0.5f, 0, 1},  {0.0f,  0.0f} },
	VertexPosUV{ { 0.5f,  0.5f, 0, 1},  {1.0f,  0.0f} },
	VertexPosUV{ {-0.5f, -0.5f, 0, 1},  {0.0f,  1.0f} },
	VertexPosUV{ { 0.5f, -0.5f, 0, 1},  {1.0f,  1.0f} }
	};


	com_ptr<ID3D11ShaderResourceView> particle_texture_srv_ = nullptr;

	MaterialFactorBuffer m_materialFactorData = {}; // 재질 상수 버퍼 데이터
	BlendState m_blendState = BlendState::AlphaBlend; // 기본 블렌드 상태
	RasterState m_rasterState = RasterState::SolidCullNone; // 기본 래스터 상태
//
//	DirectX::BoundingBox m_boundingBox;
//	DirectX::BoundingBox m_localBoundingBox;
//#ifdef _DEBUG
//	bool m_renderBoundingBox = true;
//	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_boundingBoxVertexShaderAndInputLayout = {}; // 경계 상자 정점 셰이더 및 입력 레이아웃
//	com_ptr<ID3D11PixelShader> m_boundingBoxPixelShader = nullptr; // 경계 상자 픽셀 셰이더
//#endif

public:
	ParticleComponent() = default;
	virtual ~ParticleComponent() override = default;
	ParticleComponent(const ParticleComponent&) = default;
	ParticleComponent& operator=(const ParticleComponent&) = default;
	ParticleComponent(ParticleComponent&&) = default;
	ParticleComponent& operator=(ParticleComponent&&) = default;

	const std::string& GetVertexShaderName() const { return m_vsShaderName; }
	void SetVertexShaderName(const std::string& vsShaderName) { m_vsShaderName = vsShaderName; }
	const std::string& GetPixelShaderName() const { return m_psShaderName; }
	void SetPixelShaderName(const std::string& psShaderName) { m_psShaderName = psShaderName; }

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
	void CreateBuffers();
};
/// ParticleComponent.h의 끝