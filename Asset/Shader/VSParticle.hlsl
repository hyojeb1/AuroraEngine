#include "CommonVS.hlsli"
#include "CommonMath.hlsli"

VS_OUTPUT_POS_UV main(VS_INPUT_POS_UV input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT_POS_UV output;
    
    float3 centerWorldPos = float3(WorldMatrix._41, WorldMatrix._42, WorldMatrix._43);
    
    float3 cameraRight = float3(ViewMatrix._11, ViewMatrix._12, ViewMatrix._13);
    float3 cameraUp = float3(ViewMatrix._21, ViewMatrix._22, ViewMatrix._23);
    
    float3 positionOffset = (cameraRight * input.Position.x + cameraUp * input.Position.y) * ImageScale;

    float4 worldPos = float4((centerWorldPos + positionOffset), 1.0f);
    
    worldPos.xyz += rand(instanceID) * rand3(instanceID, SpreadRadius) * SpreadDistance * EclipsedTime;
    
    worldPos.xyz = mul(worldPos.xyz, (float3x3)WorldMatrix);
    
    output.Position = mul(worldPos, VPMatrix);
    output.UV = (input.UV * UVScale) + UVOffset;

    return output;
}