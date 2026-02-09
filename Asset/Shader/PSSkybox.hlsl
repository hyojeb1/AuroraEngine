/// PSSkybox.hlsl의 시작
#include "CommonPS.hlsli"

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 ViewDir : TEXCOORD0;
};

PS_SCENE_OUTPUT main(PSInput input)
{
    PS_SCENE_OUTPUT output;
    output.Color = skyboxTexture.Sample(SamplerLinearWrap, normalize(input.ViewDir));
    output.ThresholdColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    return output;
}
/// PSSkybox.hlsl의 끝