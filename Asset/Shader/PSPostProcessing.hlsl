#include "CommonPS.hlsli"


float3 GetLutColor(float3 colorIN, Texture2D lutTex, SamplerState lutSam)
{
    float2 LutSize = float2(0.00390625f, 0.0625f); //(1 / 256, 1 / 16)
    
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
    
    color.rgb = GetLutColor(color.rgb, lutTexture, SamplerLinearClamp);
    
    return color;
}