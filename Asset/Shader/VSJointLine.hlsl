/// VSJointLine.hlsl의 시작
#include "CommonVS.hlsli"

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
};

VS_OUTPUT main(VS_INPUT_POS input)
{
    VS_OUTPUT output;
    output.Position = mul(input.Position, VPMatrix);
    return output;
}
/// VSJointLine.hlsl의 끝