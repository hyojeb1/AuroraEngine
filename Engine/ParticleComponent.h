#pragma once
#include "ComponentBase.h"
#include "Resource.h"

enum class BillboardType
{
	None,		// 빌보드 안 함 (일반 메쉬처럼 월드 회전 적용)
	Spherical,		// 구형 빌보드 (항상 카메라 정면 응시)
	Cylindrical		// 원통형 빌보드 (Y축은 고정, 나무/풀 등에 적합)
};

class ParticleComponent : public ComponentBase
{
protected:
	struct VertexPosUV
	{
		DirectX::XMFLOAT4 position = {};
		DirectX::XMFLOAT2 UV = {};
	};

	com_ptr<ID3D11Buffer> m_vertexBuffer = nullptr;

	com_ptr<ID3D11DeviceContext> m_deviceContext = nullptr; // 디바이스 컨텍스트
	const WorldNormalBuffer* m_worldNormalData = nullptr; // 월드, 월드 역행렬 상수 버퍼 데이터

	com_ptr<ID3D11Buffer> m_worldNormalBuffer = nullptr; // 월드, 월드 역행렬 상수 버퍼

	int m_particleAmount = 1;
	ParticleBuffer uv_buffer_data_ = {};
	float m_elapsedTime = 0.0f;
	float m_particleTotalTime = 1.0f;
	bool m_killOwnerAfterFinish = false;
	DirectX::XMFLOAT4 m_particleEmissionColor = { 0.0f, 0.0f, 0.0f, 1.0f };

	com_ptr<ID3D11Buffer> m_particleBuffer = nullptr; // 파티클 상수 버퍼
	com_ptr<ID3D11Buffer> m_particleEmissionBuffer = nullptr; // 파티클 에미션 상수 버퍼

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

	std::array<VertexPosUV, 4> quad_ =
	{
		VertexPosUV{ {-0.5f,  0.5f, 0, 1},  {0.0f,  0.0f} },
		VertexPosUV{ { 0.5f,  0.5f, 0, 1},  {1.0f,  0.0f} },
		VertexPosUV{ {-0.5f, -0.5f, 0, 1},  {0.0f,  1.0f} },
		VertexPosUV{ { 0.5f, -0.5f, 0, 1},  {1.0f,  1.0f} }
	};

	com_ptr<ID3D11ShaderResourceView> particle_texture_srv_ = nullptr;

	BlendState m_blendState = BlendState::AlphaBlend; // 기본 블렌드 상태
	RasterState m_rasterState = RasterState::SolidCullNone; // 기본 래스터 상태

	std::string texture_file_name_ = "Crosshair.png";

	BillboardType billboard_type_ = BillboardType::Spherical;

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

	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return true; }
	bool NeedsRender() const override { return true; }

protected:
	void Initialize() override;
	void Update() override;
	void Render() override;
	#ifdef _DEBUG
	void RenderImGui() override;
	#endif

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	virtual void CreateShaders();
	virtual void CreateBuffers();

	virtual std::string GetBillboardVSName(BillboardType type);
};