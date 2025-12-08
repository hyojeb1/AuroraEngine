Texture2D sceneTexture : register(t0);
SamplerState sceneSampler : register(s0);

struct PixelInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

float4 main(PixelInput input) : SV_TARGET
{
    return sceneTexture.Sample(sceneSampler, input.UV);
}