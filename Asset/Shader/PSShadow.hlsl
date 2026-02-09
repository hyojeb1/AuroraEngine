#include "CommonPS.hlsli"
#include "CommonMath.hlsli"

void main(PS_INPUT_STD input)
{
    //후보2
    // 텍스처 샘플링
    float4 baseColor = baseColorTexture.Sample(SamplerLinearWrap, input.UV);// * BaseColorFactor;
    //float4 baseColor = baseColorTexture.Sample(SamplerLinearWrap, input.UV) * BaseColorFactor;
    clip(baseColor.a - 0.1f); // 알파 테스트
}