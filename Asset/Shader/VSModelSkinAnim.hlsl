/// VSModelSkinAnim.hlsl의 시작
#include "CommonVS.hlsli"
#include "CommonSkinning.hlsli"

VS_OUTPUT_STD main(VS_INPUT_STD_ANIM input)
{
    VS_OUTPUT_STD output;
    
    float4 pos = Skinning(input.Position, input.BlendWeights, input.BlendIndices);
    float4 nrm = Skinning(float4(input.Normal.xyz,1), input.BlendWeights, input.BlendIndices);
    
    output.WorldPosition = mul(pos, WorldMatrix);
    output.Position = mul(output.WorldPosition, VPMatrix);
    
    output.UV = input.UV;
    
    //output.TBN[2] = normalize(mul(float4(input.Normal, 0.0f), NormalMatrix).xyz); // 이거 normalize 꼭 필요하나?
    //output.TBN[0] = normalize(mul(float4(input.Tangent, 0.0f), NormalMatrix).xyz);
    //output.TBN[1] = cross(output.TBN[2], output.TBN[0]);
    
    output.TBN = mul(float3x3(input.Tangent, input.Bitangent, nrm.xyz), (float3x3) NormalMatrix);
    
    return output;
}
/// VSModelSkinAnim.hlsl의 끝