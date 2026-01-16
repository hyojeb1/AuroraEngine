#include "CommonPS.hlsli"

float4 main(PS_INPUT_POS_COLOR input) : SV_TARGET
{
    return input.Color;
}