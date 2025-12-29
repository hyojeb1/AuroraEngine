TextureCube environmentMap : register(t1);
SamplerState samplerState : register(s1);

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 ViewDir : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    return environmentMap.Sample(samplerState, normalize(input.ViewDir));
}