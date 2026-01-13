/// CommonMath.hlsli의 시작
/// [효제] 졸려서 검토 못하고 제미나이가 짠 코드 바로 던진거임.
/// 검토 필요

#ifndef __COMMON_MATH_HLSLI__
#define __COMMON_MATH_HLSLI__

// --------------------------------------------------------
// 1. 수학 상수 (Math Constants)
// --------------------------------------------------------
static const float PI = 3.14159265359f;
static const float TWO_PI = 6.28318530718f;
static const float INV_PI = 0.31830988618f;
static const float EPSILON = 1e-6f;

static const float GAMMA = 2.2f;
static const float INV_GAMMA = 1.0f / GAMMA;

// --------------------------------------------------------
// 2. 색상 공간 변환 (Color Space Conversion)
// --------------------------------------------------------

// 선형 공간 -> sRGB 공간 (모니터 출력용, Gamma Correction)
// 보통 픽셀 셰이더의 맨 마지막 리턴 직전에 사용
float3 LinearToSRGB(float3 color)
{
    // 근사치 (pow 1/2.2) 사용. 정확한 sRGB 변환 공식보다 빠름.
    return pow(abs(color), INV_GAMMA);
}

// sRGB 공간 -> 선형 공간 (텍스처 로드 직후)
// 알베도 텍스처 등 색상 데이터는 연산 전에 반드시 선형 공간으로 변환해야 함
float3 SRGBToLinear(float3 color)
{
    return pow(abs(color), GAMMA);
}

// 휘도(Luminance) 계산 (인간의 눈이 느끼는 밝기)
float GetLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// --------------------------------------------------------
// 3. 노말 맵핑 유틸리티 (Normal Mapping Utils)
// --------------------------------------------------------

// [0, 1] 범위의 텍스처 색상을 [-1, 1] 범위의 벡터로 변환
// + 노말 강도 조절 (intensity) 기능 추가
// TBN 행렬을 사용하여 로컬 공간 노말을 월드 공간 노말로 변환
float3 UnpackNormal(float3 packedNormal, float3x3 TBN, float intensity = 1.0f)
{
    float3 localNormal = packedNormal * 2.0f - 1.0f;
    localNormal.xy *= intensity; // 노말 강도 조절
    localNormal = normalize(localNormal);
    
    return normalize(mul(localNormal, TBN));
}

// --------------------------------------------------------
// 4. 기타 유틸리티 (Misc Utils)
// [효제] 여기다가 만들것인가... 따로 만들 것이가...
// --------------------------------------------------------

// 값의 범위를 재조정 (Remap)
// value가 iMin~iMax 사이일 때, oMin~oMax 사이의 값으로 변환
float Remap(float value, float iMin, float iMax, float oMin, float oMax)
{
    return oMin + (value - iMin) * (oMax - oMin) / (iMax - iMin);
}

// --------------------------------------------------------
// 5. PBR 관련 수학 함수 (PBR Math Functions)
// --------------------------------------------------------

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

#endif // __COMMON_MATH_HLSLI__
/// CommonMath.hlsli의 끝