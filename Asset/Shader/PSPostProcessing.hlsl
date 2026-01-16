/// PSPostProcessing.hlsl의 시작
#include "CommonPS.hlsli"

float4 main(PS_INPUT_POS_UV input) : SV_TARGET
{
    // 화면 중앙에 녹색 점
    //if (input.UV.x < 0.505f && input.UV.x > 0.495f && input.UV.y < 0.51f && input.UV.y > 0.49f) return float4(0.0f, 1.0f, 0.0f, 1.0f);
    
    return sceneTexture.Sample(SamplerPointClamp, input.UV);
}

/// PSPostProcessing.hlsl의 끝