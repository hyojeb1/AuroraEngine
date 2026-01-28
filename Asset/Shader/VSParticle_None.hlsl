#include "CommonVS.hlsli"
#include "CommonMath.hlsli"

VS_OUTPUT_POS_UV main(VS_INPUT_POS_UV input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT_POS_UV output;
    
    float4 worldPos = mul(input.Position, WorldMatrix);
    
    float rndTime = Rand(WangHash(instanceID));
    float rndDirSeed = Rand(WangHash(instanceID + 1));
    float rndMag = Rand(WangHash(instanceID + 2));
    
    float randomTime = fmod(EclipsedTime + rndTime, 1.0f);
    float3 dir = Rand3(rndDirSeed, SpreadRadius);
    float3 localOffset = dir * (rndMag * SpreadDistance * randomTime);
    
    worldPos.xyz += mul(localOffset, (float3x3) WorldMatrix);
    
    output.Position = mul(worldPos, VPMatrix);
    output.UV = (input.UV * UVScale) + UVOffset;

    return output;
}