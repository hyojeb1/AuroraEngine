#include "CommonPS.hlsli"
#include "CommonMath.hlsli"

float4 main(PS_INPUT_STD input) : SV_TARGET
{
    float3 V = normalize(CameraPosition.xyz - input.WorldPosition.xyz); // 뷰 벡터
    float3 L = -LightDirection.xyz; // 라이트 벡터
    float3 H = normalize(V + L); // 하프 벡터
    
    float3 V_TBN = normalize(mul(V, input.TBN)); // 뷰 벡터를 탄젠트 공간으로 변환
    float height = normalTexture.Sample(SamplerLinearWrap, input.UV).a * HeightScale;
    float2 parallaxUV = input.UV - V_TBN.xy * height; // 시차 매핑으로 UV 오프셋 계산
    
    // 텍스처 샘플링 (오프셋된 UV 사용)
    // 알베도 텍스처
    float4 albedo = albedoTexture.Sample(SamplerLinearWrap, parallaxUV) * AlbedoFactor;
    // ORM 텍스처 // R = Ambient Occlusion, G = Roughness, B = Metallic
    float3 orm = ORMTexture.Sample(SamplerLinearWrap, parallaxUV).xyz * float3(AmbientOcclusionFactor, RoughnessFactor, MetallicFactor);
    // 노말 텍스처 // RGB = 노말, A = 높이
    float4 bump = normalTexture.Sample(SamplerLinearWrap, parallaxUV);
    
    float3 N = UnpackNormal(bump.rgb, input.TBN, NormalScale); // 노말 벡터
    float3 R = reflect(-V, N); // 반사 벡터
    
    float NdotL = saturate(dot(N, L)); // N과 L의 내적
    float NdotV = saturate(dot(N, V)); // N과 V의 내적
    float NdotH = saturate(dot(N, H)); // N과 H의 내적
    float VdotH = saturate(dot(V, H)); // V와 H의 내적
    
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.rgb, orm.b); // 금속도에 따른 F0 계산
    
    float NDF = DistributionGGX(NdotH, orm.g); // 분포 함수
    float G = GeometrySmith(NdotL, NdotV, orm.g); // 지오메트리 함수
    float3 F = FresnelSchlick(VdotH, F0); // 프레넬 효과
    
    float3 numerator = NDF * G * F; // 분자
    float denominator = 4.0f * NdotV * NdotL + 0.0001f; // 분모
    float3 specular = numerator / denominator; // 스페큘러 반사
    
    // 섀도우 맵 샘플링 // TODO: 나중에 함수로 빼야함
    float4 lightSpacePos = mul(input.WorldPosition, LightViewProjectionMatrix);
    float2 shadowTexCoord = float2(lightSpacePos.x * 0.5f + 0.5f, -lightSpacePos.y * 0.5f + 0.5f);
    float currentDepth = lightSpacePos.z * 0.999f;
    float shadow = directionalShadowMapTexture.SampleCmpLevelZero(SamplerComparisonClamp, shadowTexCoord, currentDepth);
    
    // 조명 계산
    float3 radiance = LightColor.rgb * shadow * LightDirection.w; // 조명 세기
    float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - orm.b); // 디퓨즈 반사
    float3 Lo = (kD * albedo.rgb * INV_PI + specular) * radiance * NdotL; // PBR 직접광
    
    // IBL 계산
    // 환경 맵에서 반사광 샘플링
    float3 envReflection = environmentMapTexture.SampleLevel(SamplerLinearWrap, R, orm.g * 8.0f).rgb;
    
    // 프레넬로 반사 강도 조절 (시야각에 따라 반사 강도 변화)
    float3 F_env = FresnelSchlickRoughness(NdotV, F0, orm.g);
    
    float3 kD_env = (1.0f - F_env) * (1.0f - orm.b); // 디퓨즈 기여도
    
    // 환경 맵에서 디퓨즈 샘플링 (높은 MIP 레벨 사용)
    float3 envDiffuse = environmentMapTexture.SampleLevel(SamplerLinearWrap, N, 4.0f).rgb;
    
    float3 indirectDiffuse = envDiffuse * albedo.rgb * kD_env; // 환경광 디퓨즈
    float3 indirectSpecular = envReflection * F_env; // 환경광 스페큘러
    
    // IBL 최종 기여도
    float3 ibl = (indirectDiffuse + indirectSpecular) * LightColor.w * orm.r; // AO 적용
    
    // 최종 색상
    albedo.rgb = (LinearToSRGB(Lo + ibl) * LightFactor) + (albedo.rgb * GlowFactor);
    
    return albedo + EmissionFactor;
}