///BOF PSOnlyAlbedo.hlsl
#include "CommonPS.hlsli"
#include "CommonMath.hlsli"

//#define ONLYALBEDO

#ifdef ONLYALBEDO

float4 main(PS_INPUT_STD input) : SV_TARGET
{

    float4 albedo = albedoTexture.Sample(SamplerLinearWrap, input.UV) * AlbedoFactor;
    
    return albedo;
}

#else //Albedo and Normal

float4 main(PS_INPUT_STD input) : SV_TARGET
{
    float3 V = normalize(CameraPosition.xyz - input.WorldPosition.xyz); // 뷰 벡터
    float3 L = -LightDirection.xyz; // 라이트 벡터
    float3 H = normalize(V + L); // 하프 벡터

    float4 albedo = albedoTexture.Sample(SamplerLinearWrap, input.UV) * AlbedoFactor;
    float4 nrm = normalTexture.Sample(SamplerLinearWrap, input.UV);
    
    float3 N = UnpackNormal(nrm.rgb, input.TBN, NormalScale); // 노말 벡터
    
    // float3 R = reflect(-V, N); // 반사 벡터 (단순 디버깅용이라 사용하지 않음)
    float NdotL = saturate(dot(N, L)); // N과 L의 내적 (Diffuse 강도)
    // float NdotV = saturate(dot(N, V)); // N과 V의 내적 (필요 시 사용)
    float NdotH = saturate(dot(N, H)); // N과 H의 내적 (Specular 강도)
    // float VdotH = saturate(dot(V, H)); // V와 H의 내적 (필요 시 사용)
    
    // 조명 계산 (PBR 대신 가벼운 Blinn-Phong 모델 사용)
    float3 radiance = LightColor.rgb * LightDirection.w; // 조명 색상 * 강도

    // 1. Diffuse (노말 맵의 굴곡에 따른 음영)
    float3 diffuse = albedo.rgb * NdotL;

    // 2. Specular (노말 맵의 디테일을 확인하기 위한 하이라이트 추가)
    // ORM 맵이 없으므로 고정된 수치 사용 (Shininess: 32 ~ 64 추천)
    float specularPower = 64.0f;
    float3 specular = pow(NdotH, specularPower) * 0.5f; // 0.5f는 스펙큘러 강도(Intensity)

    // 3. Ambient (완전한 암흑 방지)
    float3 ambient = albedo.rgb * 0.1f;

    // 최종 색상 합성
    float3 finalColor = (diffuse + specular) * radiance + ambient;
    
    return float4(finalColor, albedo.a);
}



#endif

///EOF PSOnlyAlbedo.hlsl