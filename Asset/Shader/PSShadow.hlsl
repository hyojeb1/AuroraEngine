#include "CommonPS.hlsli"
#include "CommonMath.hlsli"

void main(PS_INPUT_STD input)
{
    float3 V = normalize(CameraPosition.xyz - input.WorldPosition.xyz); // 뷰 벡터
    float3 V_TBN = normalize(mul(V, input.TBN)); // 뷰 벡터를 탄젠트 공간으로 변환
    float height = normalTexture.Sample(SamplerLinearWrap, input.UV).a * HeightScale;
    
    float2 parallaxUV = input.UV - V_TBN.xy * height; // 시차 매핑으로 UV 오프셋 계산
    
    // 텍스처 샘플링 (오프셋된 UV 사용)
    float4 albedo = albedoTexture.Sample(SamplerLinearWrap, parallaxUV) * AlbedoFactor;
    clip(albedo.a - 0.1f); // 알파 테스트
}