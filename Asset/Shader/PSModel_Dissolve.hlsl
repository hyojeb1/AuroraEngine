///BOF PSModel_Dissolve.hlsl
///ref PSModel, only diff from CalcDissolveEdge()
///
#include "CommonPS.hlsli"
#include "CommonMath.hlsli"

// 디졸브 계산 함수 (맛있는 효과를 위해 추가)
// noiseVal: 노이즈 텍스처 값
// threshold: 현재 디졸브 진행도 (0~1)
// width: 엣지 두께
// color: 엣지 색상
// intensity: 엣지 발광 강도
float3 CalcDissolveEdge(float noiseVal, float threshold, float width, float3 color, float intensity)
{
    // 1. Clip 처리 (threshold보다 작으면 버림)
    // -threshold를 하는 이유는 HLSL clip이 음수일 때 픽셀을 버리기 때문
    clip(noiseVal - threshold);

    // 2. Edge Glow 계산 (Smoothstep 사용)
    // 노이즈 값이 threshold에 가까울수록(타들어가는 부분) 1.0, 멀어질수록 0.0
    // step 대신 smoothstep을 써서 부드러운 발광 효과를 냄
    float edgeFactor = 1.0f - smoothstep(threshold, threshold + width, noiseVal);
    
    // 3. 엣지 팩터를 제곱하여 발광이 경계면에 더 집중되도록 함 (선택 사항)
    edgeFactor = pow(edgeFactor, 3.0f);

    return color * edgeFactor * intensity;
}



PS_SCENE_OUTPUT main(PS_INPUT_STD input)
{
    // --- [Dissolve Logic Start] ---
    // PBR 연산 전에 먼저 처리하여 불필요한 픽셀은 빨리 버림 (Early Discard)
    
    // 노이즈 텍스처 샘플링 (보통 타일링을 위해 UV에 스케일을 곱하기도 함)
    float noiseValue =  noiseTexture.Sample(SamplerLinearWrap, input.UV).r;
   
    // 디졸브 엣지 발광 색상 계산
    float3 dissolveEmission = float3(0, 0, 0);
    
    // DissolveThreshold가 0보다 클 때만 계산 (성능 최적화 및 기본 상태 유지)
    if (DissolveThreshold > 0.0f)
    {
        dissolveEmission = CalcDissolveEdge(
            noiseValue,
            DissolveThreshold,
            DissolveEdgeWidth,
            DissolveEdgeColor.rgb,
            DissolveEdgeIntensity
        );
    }
    // --- [Dissolve Logic End] ---
    
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
    float3 emission = emissionTexture.SampleLevel(SamplerLinearWrap, input.UV, distanceFromCamera).rgb * EmissionFactor.rgb; // 나중에 w값 LOD로 쓸까?
    
    // [중요] 디졸브 발광을 최종 Emission에 추가
    emission += dissolveEmission;
    
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
    //output.Color = float4(noiseValue, noiseValue, noiseValue,1 );
    output.ThresholdColor = float4(output.Color.rgb - 0.75f, baseColor.a);
    
    return output;
}