///BOF PSOnlyAlbedo.hlsl
#include "CommonPS.hlsli"
#include "CommonMath.hlsli"

float4 main(PS_INPUT_STD input) : SV_TARGET
{

    float4 albedo = albedoTexture.Sample(SamplerLinearWrap, input.UV) * AlbedoFactor;
    
    return albedo;
}

///EOF PSOnlyAlbedo.hlsl