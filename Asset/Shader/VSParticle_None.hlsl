#include "CommonVS.hlsli"
#include "CommonMath.hlsli"

VS_OUTPUT_POS_UV main(VS_INPUT_POS_UV input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT_POS_UV output;
    
    float4 worldPos = mul(input.Position, WorldMatrix);
    worldPos.xyz += mul(rand(instanceID) * rand3(instanceID, SpreadRadius) * SpreadDistance * EclipsedTime, (float3x3) WorldMatrix);
    
    output.Position = mul(worldPos, VPMatrix);
    output.UV = (input.UV * UVScale) + UVOffset;

    return output;
}