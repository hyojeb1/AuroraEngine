/// VSParticle.hlsl의 시작
#include "CommonVS.hlsli"

VS_OUTPUT_STD main(VS_INPUT_POS_UV input)
{
    VS_OUTPUT_STD output;

    float3 centerWorldPos = float3(WorldMatrix._41, WorldMatrix._42, WorldMatrix._43);

    float3 cameraRight = float3(ViewMatrix._11, ViewMatrix._12, ViewMatrix._13);
    float3 cameraUp = float3(ViewMatrix._21, ViewMatrix._22, ViewMatrix._23);
    
    float scaleX = length(float3(WorldMatrix._11, WorldMatrix._12, WorldMatrix._13));
    float scaleY = length(float3(WorldMatrix._21, WorldMatrix._22, WorldMatrix._23));

    float3 positionOffset = (input.Position.x * scaleX * cameraRight) +
                            (input.Position.y * scaleY * cameraUp);

    output.WorldPosition = float4(centerWorldPos + positionOffset, 1.0f);
    output.Position = mul(output.WorldPosition, VPMatrix);

    //output.UV = input.UV;
    output.UV = (input.UV * UVScale) + UVOffset;
    
    float3 N = normalize(cross(cameraUp, cameraRight)); 
    output.TBN = float3x3(cameraRight, cameraUp, N);

    return output;
}
/// VSParticle.hlsl의 끝