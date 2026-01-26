#include "CommonPS.hlsli"

float4 main(PS_INPUT_POS_UV input) : SV_TARGET
{
    float4 color = sceneTexture.Sample(SamplerPointClamp, float3(input.UV, 0.0f));
    
    color.rgb = pow(color.rgb, Gamma); // 감마 보정
    
    if (PostProcessingFlags & 0x1) // 그레이스케일
    {
        float gray = dot(color.rgb, float3(0.299f, 0.587f, 0.114f));
        color.rgb = lerp(color.rgb, float3(gray, gray, gray), saturate(GrayScaleIntensity));
    }
    
    return color;
}