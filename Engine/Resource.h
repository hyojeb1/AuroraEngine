/// Resource.h의 시작
#pragma once

struct RenderTarget
{
	com_ptr<ID3D11Texture2D> renderTarget = nullptr; // 렌더 타겟 텍스처
	com_ptr<ID3D11RenderTargetView> renderTargetView = nullptr; // 렌더 타겟 뷰

	com_ptr<ID3D11Texture2D> depthStencilTexture = nullptr; // 깊이-스텐실 텍스처
	com_ptr<ID3D11DepthStencilView> depthStencilView = nullptr; // 깊이-스텐실 뷰
};

constexpr UINT DIRECTIAL_LIGHT_SHADOW_MAP_SIZE = 8192; // 방향성 광원 그림자 맵 크기
enum class RenderStage
{
	DirectionalLightShadow,
	Scene,
	BackBuffer,

	Count
};

enum class DepthStencilState
{
	Default,
	Skybox,

	Count
};
constexpr std::array<D3D11_DEPTH_STENCIL_DESC, static_cast<size_t>(DepthStencilState::Count)> DEPTH_STENCIL_DESC_TEMPLATES =
{
	// Default
	D3D11_DEPTH_STENCIL_DESC
	{
		.DepthEnable = TRUE, // 깊이 테스트 활성화
		.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL, // 깊이 쓰기 활성화
		.DepthFunc = D3D11_COMPARISON_LESS, // 깊이 비교 함수: 작음
		.StencilEnable = FALSE, // 스텐실 테스트 비활성화
		.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
		.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
		.FrontFace = {}, // 사용 안 함
		.BackFace = {} // 사용 안 함
	},

	// Skybox
	D3D11_DEPTH_STENCIL_DESC
	{
		.DepthEnable = TRUE, // 깊이 테스트 활성화
		.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO, // 깊이 쓰기 활성화
		.DepthFunc = D3D11_COMPARISON_LESS_EQUAL, // 깊이 비교 함수: 작거나 같음
		.StencilEnable = FALSE, // 스텐실 테스트 비활성화
		.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
		.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
		.FrontFace = {}, // 사용 안 함
		.BackFace = {} // 사용 안 함
	}
};

enum class BlendState
{
	Opaque,
	AlphaToCoverage,
	AlphaBlend,

	Count
};
constexpr std::array<D3D11_BLEND_DESC, static_cast<size_t>(BlendState::Count)> BLEND_DESC_TEMPLATES =
{
	// Opaque
	D3D11_BLEND_DESC
	{
		.AlphaToCoverageEnable = FALSE, // 알파 투 커버리지 비활성화
		.IndependentBlendEnable = FALSE,
		.RenderTarget =
		{
			D3D11_RENDER_TARGET_BLEND_DESC
			{
				.BlendEnable = FALSE,
				.SrcBlend = D3D11_BLEND_ONE,
				.DestBlend = D3D11_BLEND_ZERO,
				.BlendOp = D3D11_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D11_BLEND_ONE,
				.DestBlendAlpha = D3D11_BLEND_ZERO,
				.BlendOpAlpha = D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
			}
		}
	},

	// AlphaToCoverage
	D3D11_BLEND_DESC
	{
		.AlphaToCoverageEnable = TRUE, // 알파 투 커버리지 활성화
		.IndependentBlendEnable = FALSE,
		.RenderTarget =
		{
			D3D11_RENDER_TARGET_BLEND_DESC
			{
				.BlendEnable = TRUE,
				.SrcBlend = D3D11_BLEND_ONE,
				.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
				.BlendOp = D3D11_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D11_BLEND_ONE,
				.DestBlendAlpha = D3D11_BLEND_ZERO,
				.BlendOpAlpha = D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
			}
		}
	},

	// AlphaBlend
	D3D11_BLEND_DESC
	{
		.AlphaToCoverageEnable = FALSE,
		.IndependentBlendEnable = FALSE,
		.RenderTarget =
		{
			D3D11_RENDER_TARGET_BLEND_DESC
			{
				.BlendEnable = TRUE,
				.SrcBlend = D3D11_BLEND_SRC_ALPHA, // 소스 알파로 소스 색상 곱하기
				.DestBlend = D3D11_BLEND_INV_SRC_ALPHA, // (1 - 소스 알파)로 대상 색상 곱하기
				.BlendOp = D3D11_BLEND_OP_ADD, // 두 값을 더하기
				.SrcBlendAlpha = D3D11_BLEND_ONE, // 소스 알파 유지
				.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA, // 대상 알파 혼합
				.BlendOpAlpha = D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
			}
		}
	}
};

enum class RasterState
{
	BackBuffer, // 백 버퍼 전용 래스터 상태 // AA 없음
	Solid,
	Wireframe,

	Count
};
constexpr std::array<D3D11_RASTERIZER_DESC, static_cast<size_t>(RasterState::Count)> RASTERIZER_DESC_TEMPLATES =
{
	// BackBuffer
	D3D11_RASTERIZER_DESC
	{
		.FillMode = D3D11_FILL_SOLID, // 실선 채우기
		.CullMode = D3D11_CULL_NONE, // 면 컬링 없음
		.FrontCounterClockwise = FALSE, // 시계방향이 앞면
		.DepthBias = 0, // 깊이 바이어스 없음
		.DepthBiasClamp = 0.0f, // 깊이 바이어스 클램프 없음
		.SlopeScaledDepthBias = 0.0f, // 기울기 기반 깊이 바이어스 없음
		.DepthClipEnable = TRUE, // 깊이 클리핑 활성화
		.ScissorEnable = FALSE, // 가위 영역 비활성화
		.MultisampleEnable = FALSE, // 멀티샘플링 비활성화
		.AntialiasedLineEnable = FALSE // 앤티앨리어싱 선 비활성화
	},

	// Solid
	D3D11_RASTERIZER_DESC
	{
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_BACK, // 뒷면 컬링(CCW)
		.FrontCounterClockwise = FALSE,
		.DepthBias = 0,
		.DepthBiasClamp = 0.0f,
		.SlopeScaledDepthBias = 0.0f,
		.DepthClipEnable = TRUE,
		.ScissorEnable = FALSE,
		.MultisampleEnable = TRUE,
		.AntialiasedLineEnable = TRUE
	},

	// Wireframe
	D3D11_RASTERIZER_DESC
	{
		.FillMode = D3D11_FILL_WIREFRAME,
		.CullMode = D3D11_CULL_BACK,
		.FrontCounterClockwise = FALSE,
		.DepthBias = 0,
		.DepthBiasClamp = 0.0f,
		.SlopeScaledDepthBias = 0.0f,
		.DepthClipEnable = TRUE,
		.ScissorEnable = FALSE,
		.MultisampleEnable = TRUE,
		.AntialiasedLineEnable = TRUE
	}
};

enum class SamplerState
{
	BackBuffer, // 백 버퍼 전용 샘플러 상태
	Default,
	ShadowMap,

	Count
};
constexpr std::array<D3D11_SAMPLER_DESC, static_cast<size_t>(SamplerState::Count)> SAMPLER_DESC_TEMPLATES =
{
	// BackBuffer
	D3D11_SAMPLER_DESC
	{
		.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR, // 선형 필터링
		.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP, // U 좌표 클램핑
		.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP, // V 좌표 클램핑
		.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP, // W 좌표 클램핑
		.MipLODBias = 0.0f, // 밉 LOD 바이어스 없음
		.MaxAnisotropy = 1, // 이방성 필터링 없음
		.ComparisonFunc = D3D11_COMPARISON_NEVER, // 비교 함수 없음
		.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f }, // 테두리 색상 (사용 안 함)
		.MinLOD = 0, // 최소 LOD
		.MaxLOD = D3D11_FLOAT32_MAX // 최대 LOD
	},

	// Default
	D3D11_SAMPLER_DESC
	{
		.Filter = D3D11_FILTER_ANISOTROPIC, // 이방성 필터링
		.AddressU = D3D11_TEXTURE_ADDRESS_WRAP, // U 좌표 래핑
		.AddressV = D3D11_TEXTURE_ADDRESS_WRAP, // V 좌표 래핑
		.AddressW = D3D11_TEXTURE_ADDRESS_WRAP, // W 좌표 래핑
		.MipLODBias = 0.0f,
		.MaxAnisotropy = 8, // 최대 이방성 필터링
		.ComparisonFunc = D3D11_COMPARISON_NEVER,
		.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
		.MinLOD = 0,
		.MaxLOD = D3D11_FLOAT32_MAX
	},

	// ShadowMap
	D3D11_SAMPLER_DESC
	{
		.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // 그림자 맵용 비교 필터링
		.AddressU = D3D11_TEXTURE_ADDRESS_BORDER, // U 좌표 테두리
		.AddressV = D3D11_TEXTURE_ADDRESS_BORDER, // V 좌표 테두리
		.AddressW = D3D11_TEXTURE_ADDRESS_BORDER, // W 좌표 테두리
		.MipLODBias = 0.0f,
		.MaxAnisotropy = 1,
		.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL, // 비교 함수: 작거나 같음
		.BorderColor = { 1.0f, 1.0f, 1.0f, 1.0f }, // 테두리 색상 흰색
		.MinLOD = 0,
		.MaxLOD = D3D11_FLOAT32_MAX
	}
};

enum class InputElement
{
	Position,
	UV,

	Normal,
	Bitangent,
	Tangent,

	Blendindex,
	Blendweight,

	Count
};
constexpr std::array<D3D11_INPUT_ELEMENT_DESC, static_cast<size_t>(InputElement::Count)> INPUT_ELEMENT_DESC_TEMPLATES =
{
	// Position
	D3D11_INPUT_ELEMENT_DESC
	{
		.SemanticName = "POSITION",
		.SemanticIndex = 0,
		.Format = DXGI_FORMAT_R32G32B32A32_FLOAT, // float4
		.InputSlot = 0,
		.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT, // 자동 오프셋 계산
		.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
		.InstanceDataStepRate = 0
	},

	// UV
	D3D11_INPUT_ELEMENT_DESC
	{
		.SemanticName = "TEXCOORD",
		.SemanticIndex = 0,
		.Format = DXGI_FORMAT_R32G32_FLOAT, // float2
		.InputSlot = 0,
		.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
		.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
		.InstanceDataStepRate = 0
	},

	// Normal
	D3D11_INPUT_ELEMENT_DESC
	{
		.SemanticName = "NORMAL",
		.SemanticIndex = 0,
		.Format = DXGI_FORMAT_R32G32B32_FLOAT, // float3
		.InputSlot = 0,
		.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
		.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
		.InstanceDataStepRate = 0
	},

	// Bitangent
	D3D11_INPUT_ELEMENT_DESC
	{
		.SemanticName = "BITANGENT",
		.SemanticIndex = 0,
		.Format = DXGI_FORMAT_R32G32B32_FLOAT, // float3
		.InputSlot = 0,
		.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
		.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
		.InstanceDataStepRate = 0
	},

	// Tangent
	D3D11_INPUT_ELEMENT_DESC
	{
		.SemanticName = "TANGENT",
		.SemanticIndex = 0,
		.Format = DXGI_FORMAT_R32G32B32_FLOAT, // float3
		.InputSlot = 0,
		.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
		.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
		.InstanceDataStepRate = 0
	},

	// Blendindex
	D3D11_INPUT_ELEMENT_DESC
	{
		.SemanticName = "BLENDINDICES",
		.SemanticIndex = 0,
		.Format = DXGI_FORMAT_R32G32B32A32_UINT, // uint4
		.InputSlot = 0,
		.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
		.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
		.InstanceDataStepRate = 0
	},

	// Blendweight
	D3D11_INPUT_ELEMENT_DESC
	{
		.SemanticName = "BLENDWEIGHT",
		.SemanticIndex = 0,
		.Format = DXGI_FORMAT_R32G32B32A32_FLOAT, // float4
		.InputSlot = 0,
		.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
		.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
		.InstanceDataStepRate = 0
	}
};

enum class VSConstBuffers
{
	ViewProjection, // ViewProjectionBuffer
	SkyboxViewProjection, // SkyboxViewProjectionBuffer

	WorldNormal, // WorldBuffer

	Time, // TimeBuffer
	Bone,

	Line,

	Count
};
struct ViewProjectionBuffer // 뷰-투영 상수 버퍼 구조체 
{
	DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixIdentity(); // 뷰 행렬 // 전치 안함
	DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixIdentity(); // 투영 행렬 // 전치 안함
	DirectX::XMMATRIX VPMatrix = DirectX::XMMatrixIdentity(); // VP 행렬 // 전치함
};
struct SkyboxViewProjectionBuffer // 스카이박스 뷰-투영 상수 버퍼 구조체
{
	DirectX::XMMATRIX skyboxVPMatrix = DirectX::XMMatrixIdentity(); // 스카이박스 VP 행렬(뷰-투영 역행렬) // 전치함
};
struct WorldNormalBuffer // 월드 및 WVP 행렬 상수 버퍼 구조체
{
	DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixIdentity(); // 월드 행렬
	DirectX::XMMATRIX normalMatrix = DirectX::XMMatrixIdentity(); // 스케일 역행렬을 적용한 월드 행렬

};
struct TimeBuffer
{
	float totalTime = 0.0f; // 누적 시간
	float deltaTime = 0.0f; // 프레임 간 시간 차이
	float sinTime = 0.0f; // 시간의 사인 값
	float cosTime = 0.0f; // 시간의 코사인 값
};
constexpr int MAX_BONES = 256;
struct BoneBuffer
{
	std::array<DirectX::XMMATRIX, MAX_BONES> boneMatrix = {}; // 본 행렬 배열
	BoneBuffer() { std::fill(boneMatrix.begin(), boneMatrix.end(), DirectX::XMMatrixIdentity()); }
};
struct LineBuffer
{
	std::array<DirectX::XMFLOAT4, 2> linePoints = { DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f}, DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f} }; // 선 시작 및 끝 점 위치
	std::array<DirectX::XMFLOAT4, 2> lineColors = { DirectX::XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f}, DirectX::XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f} }; // 선 시작 및 끝 색상
};
constexpr std::array<D3D11_BUFFER_DESC, static_cast<size_t>(VSConstBuffers::Count)> VS_CONST_BUFFER_DESCS =
{
	// ViewProjectionBuffer
	D3D11_BUFFER_DESC
	{
		.ByteWidth = sizeof(ViewProjectionBuffer),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	},

	// SkyboxViewProjectionBuffer
	D3D11_BUFFER_DESC
	{
		.ByteWidth = sizeof(SkyboxViewProjectionBuffer),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	},

	// WorldBuffer
	D3D11_BUFFER_DESC
	{
		.ByteWidth = sizeof(WorldNormalBuffer),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	},

	// TimeBuffer
	D3D11_BUFFER_DESC
	{
		.ByteWidth = sizeof(TimeBuffer),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	},

	// BoneBuffer
	D3D11_BUFFER_DESC
	{
		.ByteWidth = sizeof(BoneBuffer),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	},

	// LineBuffer
	D3D11_BUFFER_DESC
	{
		.ByteWidth = sizeof(LineBuffer),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	}
};

enum class PSConstBuffers
{
	CameraPosition, // CameraPositionBuffer
	GlobalLight, // GlobalLightBuffer
	MaterialFactor, // MaterialFactorBuffer

	Count
};
struct CameraPositionBuffer // 카메라 위치 상수 버퍼 구조체
{
	DirectX::XMVECTOR cameraPosition = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // 카메라 월드 좌표 위치
};
struct GlobalLightBuffer // 방향광 상수 버퍼 구조체
{
	DirectX::XMFLOAT4 lightColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 방향광 색상 // w는 IBL 강도
	DirectX::XMVECTOR lightDirection = DirectX::XMVectorSet(-0.5f, -1.0f, -0.5f, 1.0f); // 방향광 방향 // w는 방향광 강도

	DirectX::XMMATRIX lightViewProjectionMatrix = DirectX::XMMatrixIdentity(); // 방향광 뷰-투영 행렬 // 전치함
};
struct MaterialFactorBuffer
{
	DirectX::XMFLOAT4 albedoFactor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 텍스처 색상이 얼마나 적용되는지

	float ambientOcclusionFactor = 1.0f; // 환경광 차폐 팩터
	float roughnessFactor = 1.0f; // 거칠기 팩터
	float metallicFactor = 1.0f; // 금속성 팩터

	float ior = 1.5f; // 굴절률 // 일반적으로 1.5f 여야 함

	float normalScale = 1.0f; // 법선 맵 스케일
	float heightScale = 0.05f; // 높이 맵 스케일

	float lightFactor = 1.0f; // 광원 영향 팩터
	float glowFactor = 0.0f; // 광원 무시 알베도 팩터 // 앰비언트랑 비슷함

	DirectX::XMFLOAT4 emissionFactor = { 0.0f, 0.0f, 0.0f, 0.0f }; // 자가 발광 색상
};
constexpr std::array<D3D11_BUFFER_DESC, static_cast<size_t>(PSConstBuffers::Count)> PS_CONST_BUFFER_DESCS =
{
	// CameraPositionBuffer
	D3D11_BUFFER_DESC
	{
		.ByteWidth = sizeof(CameraPositionBuffer),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	},

	// DirectionalLightBuffer
	D3D11_BUFFER_DESC
	{
		.ByteWidth = sizeof(GlobalLightBuffer),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	},

	// MaterialFactorBuffer
	D3D11_BUFFER_DESC
	{
		.ByteWidth = sizeof(MaterialFactorBuffer),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	}
};

enum class TextureSlots
{
	BackBuffer,
	Environment,
	DirectionalLightShadow,

	Albedo, // RGBA
	ORM, // ambient occlusion(R) + roughness(G) + metallic(B)
	Normal, // XYZ(RGB) + height(A)

	Count
};

struct Vertex
{
	DirectX::XMFLOAT4 position = {};
	DirectX::XMFLOAT2 UV = {};
	DirectX::XMFLOAT3 normal = {};
	DirectX::XMFLOAT3 bitangent = {};
	DirectX::XMFLOAT3 tangent = {};

	// 스키닝 데이터 추가 (정적 메쉬는 0으로 초기화됨)
	std::array<uint32_t, 4> boneIndex = { 0, 0, 0, 0 };
	DirectX::XMFLOAT4 boneWeight = { 0.f, 0.f, 0.f, 0.f };
};


struct BoneInfo
{
	uint32_t id = 0;
	DirectX::XMFLOAT4X4 offset_matrix = {};
};

struct VectorKeyframe
{
	float time_position = 0.0f;
	DirectX::XMFLOAT3 value = { 0.0f, 0.0f, 0.0f };
};

struct QuaternionKeyframe
{
	float time_position = 0.0f;
	DirectX::XMFLOAT4 value = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct BoneAnimationChannel
{
	std::string boneName = {};
	int boneIndex = -1;
	std::vector<VectorKeyframe> position_keys = {};
	std::vector<QuaternionKeyframe> rotation_keys = {};
	std::vector<VectorKeyframe> scale_keys = {};
};

struct SkeletonNode
{
	std::string name = {};
	DirectX::XMFLOAT4X4 localTransform = {};
	int boneIndex = -1;
	std::vector<std::shared_ptr<SkeletonNode>> children = {};
};

struct Skeleton
{
	std::unordered_map<std::string, uint32_t> boneMapping = {};
	std::vector<BoneInfo> bones = {};
	DirectX::XMFLOAT4X4 globalInverseTransform = {};
	std::shared_ptr<SkeletonNode> root = nullptr;
};

struct AnimationClip
{
	static constexpr float DEFAULT_FPS = 24.0f;
	std::string name = {};
	float duration = 0.0f;
	float ticks_per_second = 0.f; // 틱 준비
	std::unordered_map<std::string, BoneAnimationChannel> channels = {};
};

struct MaterialTexture
{
	com_ptr<ID3D11ShaderResourceView> albedoTextureSRV = nullptr;
	com_ptr<ID3D11ShaderResourceView> ORMTextureSRV = nullptr;
	com_ptr<ID3D11ShaderResourceView> normalTextureSRV = nullptr;
};

struct Mesh
{
	D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	std::vector<Vertex> vertices = {};
	std::vector<UINT> indices = {};
	UINT indexCount = 0;

	DirectX::BoundingBox boundingBox = {};

	com_ptr<ID3D11Buffer> vertexBuffer = nullptr;
	com_ptr<ID3D11Buffer> indexBuffer = nullptr;

	MaterialTexture materialTexture = {};
};

enum class ModelType
{
	Static,
	Skinned,
	Rigid,
	Count
};

struct Model
{
	ModelType type = ModelType::Static;
	std::vector<Mesh> meshes = {};
	DirectX::BoundingBox boundingBox = {};

	// 애니메이션 데이터 (정적 모델인 경우 비워둠)
	Skeleton skeleton = {};
	std::vector<AnimationClip> animations = {};
};

constexpr std::array<std::pair<size_t, size_t>, 12> BOX_LINE_INDICES =
{
	std::pair<size_t, size_t>{ 0, 1 }, std::pair<size_t, size_t>{ 1, 2 }, std::pair<size_t, size_t>{ 2, 3 }, std::pair<size_t, size_t>{ 3, 0 },
	std::pair<size_t, size_t>{ 4, 5 }, std::pair<size_t, size_t>{ 5, 6 }, std::pair<size_t, size_t>{ 6, 7 }, std::pair<size_t, size_t>{ 7, 4 },
	std::pair<size_t, size_t>{ 0, 4 }, std::pair<size_t, size_t>{ 1, 5 }, std::pair<size_t, size_t>{ 2, 6 }, std::pair<size_t, size_t>{ 3, 7 }
};

/// Resource.h의 끝