#include "CommonVS.hlsli"
#include "CommonMath.hlsli"

VS_OUTPUT_POS_UV main(VS_INPUT_POS_UV input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT_POS_UV output;
    
    float3 centerWorldPos = float3(WorldMatrix._41, WorldMatrix._42, WorldMatrix._43);
    
    float3 viewRight = float3(ViewMatrix._11, ViewMatrix._12, ViewMatrix._13);
    float3 flatRight = normalize(float3(viewRight.x, 0.0f, viewRight.z));
    
    float3 positionOffset = (flatRight * input.Position.x + float3(0.0f, 1.0f, 0.0f) * input.Position.y) * ImageScale;
    
    float4 worldPos = float4((centerWorldPos + positionOffset), 1.0f);
    worldPos.xyz += mul(rand(instanceID) * rand3(instanceID, SpreadRadius) * SpreadDistance * EclipsedTime, (float3x3)WorldMatrix);
    
    output.Position = mul(worldPos, VPMatrix);
    output.UV = (input.UV * UVScale) + UVOffset;

    return output;
}