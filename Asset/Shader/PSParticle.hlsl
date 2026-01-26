/// PSParticle.hlsl의 시작
#include "CommonPS.hlsli"

float4 main(PS_INPUT_STD input) : SV_TARGET
{
    float4 color = baseColorTexture.Sample(SamplerLinearWrap, input.UV); // * BaseColorFactor;

    // if (color.a < 0.1f) discard; 
    
    //float4 emission = emissionTexture.Sample(SamplerLinearWrap, input.UV) * EmissionFactor;
    // color += emission;

    return color;
}
/// PSParticle.hlsl의 끝