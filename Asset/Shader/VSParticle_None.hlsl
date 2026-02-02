#include "CommonVS.hlsli"
#include "CommonMath.hlsli"

VS_OUTPUT_POS_UV main(VS_INPUT_POS_UV input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT_POS_UV output;
    
    float4 worldPos = mul(input.Position, WorldMatrix);
    
    float rndDirSeed = Rand(LowBias32(instanceID + 1u));
    float rndMag = Rand(LowBias32(instanceID + 2u));
    
    float3 dir = Rand3(rndDirSeed, SpreadRadius);
    float3 localOffset = dir * (rndMag * SpreadDistance * EclipsedTime);
    
    worldPos.xyz += mul(localOffset, (float3x3) WorldMatrix);
    
    output.Position = mul(worldPos, VPMatrix);
    output.UV = (input.UV * UVScale) + UVOffset;

    return output;
}