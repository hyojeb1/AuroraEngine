#include "CommonPS.hlsli"



float4 main(PS_INPUT_POS_UV input) : SV_TARGET
{
    float4 color = sceneTexture.Sample(SamplerLinearClamp, float3(input.UV, 0.0f));
    
    float4 tresholdColor;
    
    [loop] // 겁나 싼 짭우시안 블러
    for (int i = 0; i < 10; ++i) tresholdColor += sceneTexture.SampleLevel(SamplerLinearClamp, float3(input.UV, 1.0f), i);
    
    color += tresholdColor * 0.1f; // 블룸 합성
    
    color.rgb = pow(saturate(color.rgb), Gamma); // 감마 보정
    
    if (PostProcessingFlags & 0x1) // 그레이스케일
    {
        float gray = dot(color.rgb, float3(0.299f, 0.587f, 0.114f));
        color.rgb = lerp(color.rgb, float3(gray, gray, gray), saturate(GrayScaleIntensity));
    }
    
    float4 color1 = lutTexture.Sample(SamplerLinearClamp, float3(input.UV, 0.0f));
    return color1;
    
    return color;
}