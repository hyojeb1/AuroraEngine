#include "CommonVS.hlsli"

VS_OUTPUT_POS_COLOR main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT_POS_COLOR output;
    
    output.Position = mul(LinePoints[vertexID], VPMatrix);
    output.Color = LineColors[vertexID];
    
    return output;
}