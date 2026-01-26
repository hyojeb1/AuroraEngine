///BOF VSParticle_None.hlsl
#include "CommonVS.hlsli"

VS_OUTPUT_STD main(VS_INPUT_POS_UV input)
{
    VS_OUTPUT_STD output;

    // 1. 일반적인 월드 변환 (회전, 크기, 위치 모두 WorldMatrix 따름)
    output.WorldPosition = mul(input.Position, WorldMatrix);
    output.Position = mul(output.WorldPosition, VPMatrix);

    //output.UV = input.UV;
    output.UV = (input.UV * UVScale) + UVOffset;
    
    // 노멀 계산 (월드 회전 적용)
    float3 N = normalize(mul(float3(0, 0, -1), (float3x3) WorldMatrix)); // Quad가 기본적으로 -Z를 본다고 가정 시
    // 혹은 일반적인 메시라면 Tangent/Binormal 계산 필요하지만, 
    // 파티클 Quad 기준으로는 대략적인 Up/Right를 잡습니다.
    
    // 간단히 월드 기준 축 사용
    float3 cameraRight = float3(ViewMatrix._11, ViewMatrix._12, ViewMatrix._13);
    float3 cameraUp = float3(ViewMatrix._21, ViewMatrix._22, ViewMatrix._23);
    output.TBN = float3x3(cameraRight, cameraUp, -cameraRight); // 임시

    return output;
}
///EOF VSParticle_None.hlsl