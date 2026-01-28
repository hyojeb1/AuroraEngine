#include "CommonVS.hlsli"

VS_OUTPUT_POS_UV main(VS_INPUT_POS_UV input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT_POS_UV output;
    
    float3 centerWorldPos = float3(WorldMatrix._41, WorldMatrix._42, WorldMatrix._43);
    
    float3 cameraRight = float3(ViewMatrix._11, ViewMatrix._12, ViewMatrix._13);
    float3 cameraUp = float3(ViewMatrix._21, ViewMatrix._22, ViewMatrix._23);
    
    float scaleX = length(float3(WorldMatrix._11, WorldMatrix._12, WorldMatrix._13));
    float scaleY = length(float3(WorldMatrix._21, WorldMatrix._22, WorldMatrix._23));
    
    float3 positionOffset = cameraRight * (input.Position.x * scaleX) + cameraUp * (input.Position.y * scaleY);

    float4 worldPos = float4((centerWorldPos + positionOffset), 1.0f);
    
    output.Position = mul(worldPos, VPMatrix);
    output.UV = (input.UV * UVScale) + UVOffset;

    return output;
}