cbuffer DirectionalLight : register(b0)
{
    float4 lightDirection; // 정규화된 방향 벡터여야 함
    float4 lightColor;
};

cbuffer MaterialFactor : register(b1)
{
    float4 albedoFactor;
    float metallicFactor;
    float roughnessFactor;
    float lightFactor;
    float ambient;
};

SamplerState textureSampler : register(s1);

TextureCube environmentMapTexture : register(t1); // 나중에 사용

Texture2D albedoTexture : register(t2);
Texture2D normalTexture : register(t3);
Texture2D metallicTexture : register(t4);
Texture2D roughnessTexture : register(t5);

struct PixelInput
{
    float4 WorldPosition : POSITION0;
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3x3 TBN : TBN0;
};

float4 main(PixelInput input) : SV_TARGET
{
    float4 albedo = albedoTexture.Sample(textureSampler, input.UV) * albedoFactor;
    float3 normalMap = normalTexture.Sample(textureSampler, input.UV).xyz * 2.0f - 1.0f;
    
    float3 normal = mul(normalMap, input.TBN);
    float3 lightDir = lightDirection.xyz;
    albedo.rgb *= lightColor.rgb * dot(normal, lightDir) * lightFactor;
    
    return albedo;
}