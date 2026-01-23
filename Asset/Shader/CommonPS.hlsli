/// CommonPS.hlsli의 시작
#ifndef __COMMON_PS_HLSLI__
#define __COMMON_PS_HLSLI__

// --------------------------------------------------------
// Constant Buffers
// --------------------------------------------------------

cbuffer PostProcessParams : register(b0)
{
    uint PostProcessingFlags;
    
    float GrayScaleIntensity;
    float MotionBlurIntensity;
    float PaddingA;
};

cbuffer CameraPosition : register(b1)
{
    float4 CameraPosition;
};

cbuffer GlobalLight : register(b2)
{
    float4 LightColor; // w는 IBL 강도
    float4 LightDirection; // 정규화된 방향 벡터 // w는 방향광 강도
    
    matrix LightViewProjectionMatrix;
};

cbuffer MaterialFactor : register(b3)
{
    float4 BaseColorFactor;
    
    float AmbientOcclusionFactor;
    float RoughnessFactor;
    float MetallicFactor;
    
    float NormalScale;
    
    float4 EmissionFactor;
};

// --------------------------------------------------------
// Sampler States
// --------------------------------------------------------

SamplerState SamplerPointClamp : register(s0); // PostProcess
SamplerState SamplerLinearWrap : register(s1); // Model, Skybox
SamplerComparisonState SamplerComparisonClamp : register(s2); // Shadow Map


// --------------------------------------------------------
// Textures
// --------------------------------------------------------
Texture2D sceneTexture : register(t0);
TextureCube environmentMapTexture : register(t1);
Texture2D directionalShadowMapTexture : register(t2);

// PBR 재질
Texture2D baseColorTexture : register(t3);
Texture2D ORMTexture : register(t4);
Texture2D normalTexture : register(t5);
Texture2D emissionTexture : register(t6);

// --------------------------------------------------------
// Input Structures (VS 출력과 매칭되어야 함)
// --------------------------------------------------------

struct PS_INPUT_STD
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION0;
    float2 UV : TEXCOORD0;
    float3x3 TBN : TBN0;
};

struct PS_INPUT_POS_UV
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

struct PS_INPUT_POS_COLOR
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
};

#endif // __COMMON_PS_HLSLI__
/// CommonPS.hlsli의 끝