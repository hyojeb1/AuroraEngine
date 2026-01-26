/// VSModelSkinAnim.hlsl의 시작
#include "CommonVS.hlsli"


float4 Skinning(float4 pos, float4 weight, uint4 index)
{
    float4 skinVtx = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // Standard Skinning Logic
    // index values are safe as long as they are < MAX_BONES (256) defined in CommonVS
    skinVtx += weight.x * mul(pos, BoneTransforms[index.x]);
    skinVtx += weight.y * mul(pos, BoneTransforms[index.y]);
    skinVtx += weight.z * mul(pos, BoneTransforms[index.z]);
    skinVtx += weight.w * mul(pos, BoneTransforms[index.w]);

    return skinVtx;
}

VS_OUTPUT_STD main(VS_INPUT_STD_ANIM input)
{
    VS_OUTPUT_STD output;
    
    float4 pos = Skinning(input.Position, input.BlendWeights, input.BlendIndices);
    float4 nrm = Skinning(float4(input.Normal.xyz,0), input.BlendWeights, input.BlendIndices);
    float3 tan = Skinning(float4(input.Tangent, 0.0f), input.BlendWeights, input.BlendIndices).xyz;
    float3 bit = Skinning(float4(input.Bitangent, 0.0f), input.BlendWeights, input.BlendIndices).xyz;
    
    output.WorldPosition = mul(pos, WorldMatrix);
    output.Position = mul(output.WorldPosition, VPMatrix);
    
    output.UV = input.UV;
    
    output.TBN = mul(float3x3(tan, bit, nrm.xyz), (float3x3) NormalMatrix);
    
    return output;
}
/// VSModelSkinAnim.hlsl의 끝