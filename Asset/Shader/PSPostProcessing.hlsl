#include "CommonPS.hlsli"

float3 GetLutColor(float3 colorIN, Texture2D lutTex, SamplerState lutSam)
{
    // 256x16 LUT 기준 상수 (1 / 256, 1 / 16)
    static const float2 LutSize = float2(0.00390625f, 0.0625f);
    
    float3 scaleColor = saturate(colorIN) * 15.0f;
    
    float blueInt = floor(scaleColor.b);
    float blueFrac = scaleColor.b - blueInt;
    
    float2 baseUV;
    baseUV.x = (scaleColor.r + 0.5f) * LutSize.x;
    baseUV.y = (scaleColor.g + 0.5f) * LutSize.y;
    
    float2 uv1 = baseUV;
    uv1.x += blueInt * LutSize.y;
    
    float2 uv2 = baseUV;
    uv2.x += (blueInt + 1.0f) * LutSize.y;
    
    float3 color1 = lutTex.SampleLevel(lutSam, uv1, 0).rgb;
    float3 color2 = lutTex.SampleLevel(lutSam, uv2, 0).rgb;
    
    return lerp(color1, color2, blueFrac);
}

float4 main(PS_INPUT_POS_UV input) : SV_TARGET
{
    float4 color = sceneTexture.Sample(SamplerLinearClamp, float3(input.UV, 0.0f));
    
    // 포스트 프로세싱 플래그에 따른 처리
    static const uint PP_BLOOM = 1u << 0;
    static const uint PP_GAMMA = 1u << 1;
    static const uint PP_GRAYSCALE = 1u << 2;
    static const uint PP_VIGNETTING = 1u << 3;
    // 블룸 합성
    if (PostProcessingFlags & PP_BLOOM)
    {
        float4 tresholdColor;
        [loop] for (int i = 1; i < 11; ++i) tresholdColor += sceneTexture.SampleLevel(SamplerLinearClamp, float3(input.UV, 1.0f), i); // 겁나 싼 짭우시안 블러
        color += tresholdColor * 0.1f * BloomIntensity;
    }
    // 감마 보정
    if (PostProcessingFlags & PP_GAMMA) color.rgb = pow(saturate(color.rgb), Gamma);
    // 그레이스케일
    if (PostProcessingFlags & PP_GRAYSCALE) color.rgb = lerp(color.rgb, dot(color.rgb, float3(0.299f, 0.587f, 0.114f)), saturate(GrayScaleIntensity));
    // 비네팅
    if (PostProcessingFlags & PP_VIGNETTING)
    {
        float2 position = input.UV - float2(0.5f, 0.5f);
        float vignette = 1.0f - length(position) * 1.4142f; // 대각선 길이로 정규화
        vignette = pow(saturate(vignette), VignettingColor.w * 16.0f);
        color.rgb = lerp(lerp(VignettingColor.rgb, color.rgb * vignette, vignette), VignettingColor.rgb, VignettingColor.w);
    }
    
    
    color.rgb = GetLutColor(color.rgb, lutTexture, SamplerLinearClamp);
    
    return color;
}