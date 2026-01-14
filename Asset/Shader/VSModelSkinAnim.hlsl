/// VSModelSkinAnim.hlsl의 시작
#include "CommonVS.hlsli"
#include "CommonSkinning.hlsli"

VS_OUTPUT_STD main(VS_INPUT_STD_ANIM input)
{
    VS_OUTPUT_STD output;
    
    float4 pos = Skinning(input.Position, input.BlendWeights, input.BlendIndices);
    float4 nrm = Skinning(float4(input.Normal.xyz,0), input.BlendWeights, input.BlendIndices);
    
    //float4 pos = input.Position;
    //float3 nrm = input.Normal.xyz;
    
    output.WorldPosition = mul(pos, WorldMatrix);
    output.Position = mul(output.WorldPosition, VPMatrix);
    
    output.UV = input.UV;
    
    output.TBN = mul(float3x3(input.Tangent, input.Bitangent, nrm.xyz), (float3x3) NormalMatrix);
    
    return output;
}
/// VSModelSkinAnim.hlsl의 끝