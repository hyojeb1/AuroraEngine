#include "CommonVS.hlsli"

VS_OUTPUT_POS_COLOR main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT_POS_COLOR output;
    
    if (instanceID < 1001) output.Color = float4(vertexID * 1000.0f - 500.0f, 0.0f, float(instanceID) - 500.0f, 1.0f);
    else if (instanceID < 2002) output.Color = float4(float(instanceID) - 1501.0f, 0.0f, vertexID * 1000.0f - 500.0f, 1.0f);
    else output.Color = float4(0.0f, vertexID * 1000.0f - 500.0f, 0.0f, 1.0f);
    
    output.Position = mul(output.Color, VPMatrix);
    if (fmod(abs(output.Color.x), 5.0f) > 0.5f || fmod(abs(output.Color.z), 5.0f) > 0.5f) output.Color = float4(0.25f, 0.25f, 0.25f, 1.0f);
    
    return output;
}