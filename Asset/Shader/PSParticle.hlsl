#include "CommonPS.hlsli"

float4 main(PS_INPUT_POS_UV input) : SV_TARGET
{
    return baseColorTexture.Sample(SamplerLinearWrap, input.UV);
}