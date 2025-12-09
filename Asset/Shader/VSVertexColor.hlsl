cbuffer PerObject : register(b0)
{
    matrix WVP;
}


struct VertexInput
{
    float4 Position : POSITION;
    float4 Color : COLOR0;
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    
    input.Position.w = 1.0f;
    output.Position = mul(input.Position, WVP);
    output.Color = input.Color;
    
    return output;
}