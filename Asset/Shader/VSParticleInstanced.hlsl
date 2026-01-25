/// VSParticleInstanced.hlsl의 시작
#include "CommonVS.hlsli"

struct VS_INPUT_PARTICLE_INSTANCED
{
    float4 Position : POSITION;
    float2 UV : TEXCOORD;

    float4 World0 : WORLD0;
    float4 World1 : WORLD1;
    float4 World2 : WORLD2;
    float4 World3 : WORLD3;
    float4 Color : COLOR0;
    float2 InstanceUVOffset : TEXCOORD1;
};

struct VS_OUTPUT_PARTICLE_INSTANCED
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION0;
    float2 UV : TEXCOORD0;
    float3x3 TBN : TBN0;
    float4 Color : COLOR0;
};

VS_OUTPUT_PARTICLE_INSTANCED main(VS_INPUT_PARTICLE_INSTANCED input)
{
    VS_OUTPUT_PARTICLE_INSTANCED output;

    float4x4 instanceWorld = float4x4(input.World0, input.World1, input.World2, input.World3);

    // 2. 인스턴스의 위치(Translation) 추출 (4행)
    float3 centerWorldPos = float3(instanceWorld._41, instanceWorld._42, instanceWorld._43);

    // 3. 인스턴스의 스케일(Scale) 추출 (각 축의 길이)
    // (단순히 행렬에서 스케일만 뽑기 위해 length 사용)
    float scaleX = length(float3(instanceWorld._11, instanceWorld._12, instanceWorld._13));
    float scaleY = length(float3(instanceWorld._21, instanceWorld._22, instanceWorld._23));

    // 4. 빌보딩 로직 (ViewMatrix의 역함수 개념 이용)
    float3 cameraRight = float3(ViewMatrix._11, ViewMatrix._12, ViewMatrix._13);
    float3 cameraUp = float3(ViewMatrix._21, ViewMatrix._22, ViewMatrix._23);

    // 5. 정점 위치 계산 (중심점 + 카메라 기준 오프셋 * 스케일)
    float3 positionOffset = (input.Position.x * scaleX * cameraRight) +
                            (input.Position.y * scaleY * cameraUp);

    output.WorldPosition = float4(centerWorldPos + positionOffset, 1.0f);
    output.Position = mul(output.WorldPosition, VPMatrix);

    // 6. UV 및 컬러 (작성하신 그대로)
    output.UV = (input.UV * UVScale) + input.InstanceUVOffset;
    output.Color = input.Color;

    // 7. Normal (빌보드니까 항상 카메라를 바라봄)
    float3 N = normalize(cross(cameraUp, cameraRight));
    output.TBN = float3x3(cameraRight, cameraUp, N);

    return output;
}
/// VSParticleInstanced.hlsl의 끝
