#pragma once

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

enum class BlendState
{
	Opaque,
	AlphaToCoverage,
	AlphaBlend,

	Count
};

enum class RasterState
{
	BackBuffer, // 백 버퍼 전용 래스터 상태 // AA 없음
	Solid,
	SolidCullNone,
	Wireframe,

	Count
};

enum class SamplerState
{
	BackBuffer, // 백 버퍼 전용 샘플러 상태
	Default,
	ShadowMap,

	Count
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

enum class VSConstBuffers
{
	ViewProjection, // ViewProjectionBuffer
	SkyboxViewProjection, // SkyboxViewProjectionBuffer

	WorldNormal, // WorldBuffer

	Time, // TimeBuffer
	Bone,

	Line,
	Particle,

	Count
};

enum class PSConstBuffers
{
	PostProcessing, // PostProcessingBuffer
	CameraPosition, // CameraPositionBuffer
	GlobalLight, // GlobalLightBuffer
	MaterialFactor, // MaterialFactorBuffer

	Count
};

enum class TextureSlots
{
	BackBuffer,
	Environment,
	DirectionalLightShadow,

	BaseColor,
	ORM,
	Normal,
	Emission,

	Count
};


enum class ModelType
{
	Static,
	Skinned,
	//Rigid,//마인크래프트
	Count
};