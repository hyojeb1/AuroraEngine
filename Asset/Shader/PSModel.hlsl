///BOF PSModel.hlsl
#include "CommonPS.hlsli"
#include "CommonMath.hlsli"

PS_SCENE_OUTPUT main(PS_INPUT_STD input)
{
    float3 cameraToPixel = CameraPosition.xyz - input.WorldPosition.xyz;
    float distanceFromCamera = length(cameraToPixel) * 0.05f;
    
    // 텍스처 샘플링
    // 베이스 컬러 텍스처
    float4 baseColor = baseColorTexture.SampleLevel(SamplerLinearWrap, input.UV, distanceFromCamera) * BaseColorFactor;
    // ORM 텍스처
    float3 orm = ORMTexture.SampleLevel(SamplerLinearWrap, input.UV, distanceFromCamera).xyz * float3(AmbientOcclusionFactor, RoughnessFactor, MetallicFactor);
    // 노말 텍스처
    float4 normal = normalTexture.SampleLevel(SamplerLinearWrap, input.UV, distanceFromCamera);
    // 방출 텍스처
    float3 emission = emissionTexture.SampleLevel(SamplerLinearWrap, input.UV, distanceFromCamera).rgb * EmissionFactor.rgb;
    
    float3 V = normalize(cameraToPixel); // 뷰 벡터
    float3 L = -LightDirection.xyz; // 라이트 벡터
    float3 H = normalize(V + L); // 하프 벡터
    float3 N = UnpackNormal(normal.rgb, input.TBN, NormalScale); // 노말 벡터
    float3 R = reflect(-V, N); // 반사 벡터
    
    float NdotL = saturate(dot(N, L)); // N과 L의 내적
    float NdotV = saturate(dot(N, V)); // N과 V의 내적
    float NdotH = saturate(dot(N, H)); // N과 H의 내적
    float VdotH = saturate(dot(V, H)); // V와 H의 내적
    
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor.rgb, orm.b); // 금속도에 따른 F0 계산
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
    float3 radiance = LightColor.rgb * LightDirection.w; // 조명 세기
    float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - orm.b); // 디퓨즈 반사
    float3 Lo = (kD * baseColor.rgb * INV_PI + specular) * radiance * NdotL * shadow; // PBR 직접광
    
    // IBL 계산
    // 환경 맵에서 반사광 샘플링
    float3 envReflection = environmentMapTexture.SampleLevel(SamplerLinearWrap, R, orm.g * 32.0f).rgb;
    
    // 프레넬로 반사 강도 조절 (시야각에 따라 반사 강도 변화)
    float3 F_env = FresnelSchlickRoughness(NdotV, F0, orm.g);
    
    float3 kD_env = (1.0f - F_env) * (1.0f - orm.b); // 디퓨즈 기여도
    
    // 환경 맵에서 디퓨즈 샘플링 (높은 MIP 레벨 사용)
    float3 envDiffuse = environmentMapTexture.SampleLevel(SamplerLinearWrap, N, orm.g * 32.0f).rgb;
    
    float3 indirectDiffuse = lerp(envDiffuse, LightColor.rgb, orm.r) * baseColor.rgb * kD_env; // 환경광 디퓨즈
    float3 indirectSpecular = envReflection * F_env; // 환경광 스페큘러
    
    // IBL 최종 기여도
    float3 ibl = (indirectDiffuse + indirectSpecular) * LightColor.w;
    
    PS_SCENE_OUTPUT output;
    output.Color = float4(Lo + ibl + emission, baseColor.a);
    output.ThresholdColor = float4(output.Color.rgb - 0.75f, baseColor.a);
   
    return output;
}