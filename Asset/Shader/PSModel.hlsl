cbuffer MaterialFactor : register(b0)
{
    float4 albedoFactor;
    float metallicFactor;
    float roughnessFactor;
    float lightFactor;
    float ambient;
};

SamplerState textureSampler : register(s0);

Texture2D albedoTexture : register(t0);
Texture2D metallicRoughnessTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D aoTexture : register(t3);

struct PixelInput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
};

float4 main(PixelInput input) : SV_TARGET
{
    return input.Color;
}