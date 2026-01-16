#include "CommonVS.hlsli"

VS_OUTPUT_POS_COLOR main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT_POS_COLOR output;
    
    if (instanceID < 101) output.Color = float4(vertexID * 100.0f - 50.0f, 0.0f, float(instanceID) - 50.0f, 1.0f);
    else if (instanceID < 202) output.Color = float4(float(instanceID) - 151.0f, 0.0f, vertexID * 100.0f - 50.0f, 1.0f);
    else output.Color = float4(0.0f, vertexID * 100.0f - 50.0f, 0.0f, 1.0f);
    
    output.Position = mul(output.Color, VPMatrix);
    output.Color.xz = abs(output.Color.xz * 0.05f);
    
    return output;
}