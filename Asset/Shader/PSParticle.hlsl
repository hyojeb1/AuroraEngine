#include "CommonPS.hlsli"

PS_SCENE_OUTPUT main(PS_INPUT_POS_UV input)
{
    PS_SCENE_OUTPUT output;
    
    output.Color = baseColorTexture.Sample(SamplerLinearWrap, input.UV) * ParticleBaseColor;
    output.ThresholdColor = output.Color * ParticleEmission; // 이거 factor으로 할지 그냥 더할지 고민중

    return output;
}