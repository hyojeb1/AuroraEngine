#include "CommonPS.hlsli"

float3 GetLutColor(float3 colorIN, Texture2D lutTex, SamplerState lutSam)
{
    // 256x16 LUT 기준 상수 (1 / 256, 1 / 16)
    static const float2 LutSize = float2(0.00390625f, 0.0625f);
    
    // 색상 범위를 0~1로 자르고 0~15 (16개 슬라이스) 범위로 확장
    // Blue 채널이 LUT의 슬라이스(Z)를 결정합니다.
    float3 scaleColor = saturate(colorIN) * 15.0f;
    
    float blueInt = floor(scaleColor.b); // 현재 Blue 슬라이스 인덱스 (정수)
    float blueFrac = scaleColor.b - blueInt; // 선형 보간을 위한 소수점 부분
    
    // 텍셀의 중심을 샘플링하기 위한 UV 계산 (Half-Texel Offset)
    // 기본 UV: (RG * 15 + 0.5) / 해상도
    float2 baseUV;
    baseUV.x = (scaleColor.r + 0.5f) * LutSize.x;
    baseUV.y = (scaleColor.g + 0.5f) * LutSize.y;
    
    // 첫 번째 슬라이스(Floor)의 U 좌표 offset
    float2 uv1 = baseUV;
    uv1.x += blueInt * LutSize.y; // 슬라이스 크기(1/16)만큼 이동
    
    // 두 번째 슬라이스(Ceil)의 U 좌표 offset
    float2 uv2 = baseUV;
    uv2.x += (blueInt + 1.0f) * LutSize.y;
    
    // 두 슬라이스를 각각 샘플링 (Mipmap 0번 고정)
    float3 color1 = lutTex.SampleLevel(lutSam, uv1, 0).rgb;
    float3 color2 = lutTex.SampleLevel(lutSam, uv2, 0).rgb;
    
    // Blue 채널의 소수점 값을 이용해 두 슬라이스 사이를 보간
    return lerp(color1, color2, blueFrac);
}

float4 main(PS_INPUT_POS_UV input) : SV_TARGET
{
    float4 color = sceneTexture.Sample(SamplerLinearClamp, float3(input.UV, 0.0f));
    
    float4 tresholdColor;
    [loop] for (int i = 0; i < 10; ++i) tresholdColor += sceneTexture.SampleLevel(SamplerLinearClamp, float3(input.UV, 1.0f), i); // 겁나 싼 짭우시안 블러
    
    color += tresholdColor * 0.1f * BloomIntensity; // 블룸 합성
    
    color.rgb = pow(saturate(color.rgb), Gamma); // 감마 보정
    
    // 그레이스케일
    if (PostProcessingFlags & 0x1) color.rgb = lerp(color.rgb, dot(color.rgb, float3(0.299f, 0.587f, 0.114f)), saturate(GrayScaleIntensity));
    
    // 비네팅
    if (PostProcessingFlags & 0x2)
    {
        float2 position = input.UV - float2(0.5f, 0.5f);
        float vignette = 1.0f - length(position) * 1.4142f; // 대각선 길이로 정규화
        vignette = pow(saturate(vignette), VignettingColor.w);
        color.rgb = lerp(color.rgb * vignette, VignettingColor.rgb, 1.0f - vignette);
    }
    
 
    color.rgb = GetLutColor(color.rgb, lutTexture, SamplerLinearClamp);
    
    return color;
}