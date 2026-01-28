#ifndef __COMMON_MATH_HLSLI__
#define __COMMON_MATH_HLSLI__

// 수학 상수
static const float PI = 3.14159265359f;
static const float TWO_PI = 6.28318530718f;
static const float INV_PI = 0.31830988618f;
static const float EPSILON = 1e-6f;

static const float GAMMA = 2.2f;
static const float INV_GAMMA = 1.0f / GAMMA;

// 유틸리티 함수

// 0~1 범위의 유사 난수 생성
float rand(float seed)
{
    return frac(sin(seed * 12.9898 + 78.233) * 43758.5453);
}

// 3D 벡터 유사 난수 생성
float3 rand3(float seed, float spread = 0.0f)
{
    if (spread <= 0.5f) return normalize(float3(rand(rand(seed)), rand(rand(rand(seed))), rand(rand(rand(rand(seed))))) - 0.5f);
    return normalize(pow(float3(rand(rand(seed)), rand(rand(rand(seed))), rand(rand(rand(rand(seed))))), rcp(spread)));
}

// 노말 맵핑 
float3 UnpackNormal(float3 packedNormal, float3x3 TBN, float intensity = 1.0f)
{
    float3 localNormal = packedNormal * 2.0f - 1.0f;
    localNormal.xy *= intensity;
    localNormal = normalize(localNormal);
    
    return normalize(mul(localNormal, TBN));
}

// PBR 관련 수학 함수

// 프레넬 효과 (Schlick 근사)
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// 프레넬 효과 (거칠기 보정된 Schlick 근사)
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// GGX 분포 함수 (Trowbridge-Reitz)
float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    
    return a2 / denom;
}

// 지오메트리 함수 (Schlick-GGX 근사)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    
    float denom = NdotV * (1.0f - k) + k;
    
    return NdotV / denom;
}

// 지오메트리 함수 (Smith)
float GeometrySmith(float NdotL, float NdotV, float roughness)
{
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    
    return ggx1 * ggx2;
}

#endif