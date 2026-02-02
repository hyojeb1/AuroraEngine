#include "CommonPS.hlsli"

PS_SCENE_OUTPUT main(PS_INPUT_POS_UV input)
{
    PS_SCENE_OUTPUT output;
    
    output.Color = baseColorTexture.Sample(SamplerLinearWrap, input.UV);
    output.ThresholdColor = output.Color;

    return output;
}