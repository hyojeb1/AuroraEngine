TextureCube environmentMap : register(t1);
SamplerState samplerState : register(s1);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 viewDir : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    return environmentMap.Sample(samplerState, normalize(input.viewDir));
}