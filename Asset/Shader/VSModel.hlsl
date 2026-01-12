/// VSModel.hlsl의 시작
#include "CommonVS.hlsli"

VS_OUTPUT_STD main(VS_INPUT_STD input)
{
    VS_OUTPUT_STD output;
    
    output.WorldPosition = mul(input.Position, WorldMatrix);
    output.Position = mul(output.WorldPosition, VPMatrix);
    
    output.UV = input.UV;
    
    output.TBN = mul(float3x3(input.Tangent, input.Bitangent, input.Normal), (float3x3)NormalMatrix);

    return output;
}
/// VSModel.hlsl의 끝