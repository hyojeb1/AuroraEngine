#include "CommonVS.hlsli"
#include "CommonPS.hlsli"

[maxvertexcount(4)]
void main(line VS_OUTPUT_POS_COLOR IN[2], inout TriangleStream<PS_INPUT_POS_COLOR> stream)
{
    float2 ndc0 = IN[0].Position.xy / IN[0].Position.w;
    float2 ndc1 = IN[1].Position.xy / IN[1].Position.w;
    
    float2 dir = ndc1 - ndc0;
    float len = length(dir);
    if (len < 1e-6f)
    {
        dir = float2(0.0f, 1.0f);
        len = 1.0f;
    }
    dir /= len;
    
    float2 perp = float2(-dir.y, dir.x);
    
    const float halfWidthNDC = 0.01f;

    float2 off = perp * halfWidthNDC;
    
    float4 v0 = float4((ndc0 + off) * IN[0].Position.w, IN[0].Position.z, IN[0].Position.w);
    float4 v1 = float4((ndc0 - off) * IN[0].Position.w, IN[0].Position.z, IN[0].Position.w);
    float4 v2 = float4((ndc1 + off) * IN[1].Position.w, IN[1].Position.z, IN[1].Position.w);
    float4 v3 = float4((ndc1 - off) * IN[1].Position.w, IN[1].Position.z, IN[1].Position.w);
    
    PS_INPUT_POS_COLOR outVert;

    outVert.Position = v0;
    outVert.Color = IN[0].Color;
    stream.Append(outVert);

    outVert.Position = v1;
    outVert.Color = IN[0].Color;
    stream.Append(outVert);

    outVert.Position = v2;
    outVert.Color = IN[1].Color;
    stream.Append(outVert);

    outVert.Position = v3;
    outVert.Color = IN[1].Color;
    stream.Append(outVert);

    stream.RestartStrip();
}