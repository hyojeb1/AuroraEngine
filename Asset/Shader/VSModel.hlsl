/// VSModel.hlsl의 시작
#include "CommonVS.hlsli"

VS_OUTPUT_STD main(VS_INPUT_STD input)
{
    VS_OUTPUT_STD output;
    
    output.WorldPosition = mul(input.Position, WorldMatrix);
    output.Position = mul(output.WorldPosition, VPMatrix);
    
    output.UV = input.UV;
    
    output.TBN[2] = normalize(mul(float4(input.Normal, 0.0f), NormalMatrix).xyz); // 이거 normalize 꼭 필요하나?
    output.TBN[1] = normalize(mul(float4(input.Bitangent, 0.0f), NormalMatrix).xyz);
    output.TBN[0] = normalize(mul(float4(input.Tangent, 0.0f), NormalMatrix).xyz);
    
    return output;
}
/// VSModel.hlsl의 끝