///BOF VSParticle_Cylindrical.hlsl
#include "CommonVS.hlsli"

VS_OUTPUT_STD main(VS_INPUT_POS_UV input)
{
    VS_OUTPUT_STD output;

    // 1. 월드 행렬에서 위치(Translation)와 스케일(Scale) 추출
    float3 centerWorldPos = float3(WorldMatrix._41, WorldMatrix._42, WorldMatrix._43);
    float scaleX = length(float3(WorldMatrix._11, WorldMatrix._12, WorldMatrix._13));
    float scaleY = length(float3(WorldMatrix._21, WorldMatrix._22, WorldMatrix._23));

    // 2. 카메라의 Right 벡터 계산 (ViewMatrix의 1행)
    // Cylindrical(원통형)은 Y축 회전을 하지 않으므로, 
    // 카메라의 Right 벡터를 XZ 평면에 투영(Project)하여 사용합니다.
    float3 viewRight = float3(ViewMatrix._11, ViewMatrix._12, ViewMatrix._13);
    
    // Y값을 0으로 죽이고 정규화 -> 항상 땅바닥과 평행한 Right 벡터 생성
    float3 flatRight = normalize(float3(viewRight.x, 0.0f, viewRight.z));

    // 3. Up 벡터는 월드 Y축(0, 1, 0)으로 고정
    float3 fixedUp = float3(0.0f, 1.0f, 0.0f);

    // 4. 오프셋 계산 (입력 정점 위치 * 스케일 * 기저 벡터)
    // X축으로는 카메라를 따라 회전(flatRight), Y축으로는 고정(fixedUp)
    float3 positionOffset = (input.Position.x * scaleX * flatRight) +
                            (input.Position.y * scaleY * fixedUp);

    output.WorldPosition = float4(centerWorldPos + positionOffset, 1.0f);
    output.Position = mul(output.WorldPosition, VPMatrix);

    output.UV = input.UV;

    // 노멀 계산 (라이팅이 필요하다면 평면의 노멀은 Right와 Up의 외적)
    float3 N = normalize(cross(fixedUp, flatRight));
    output.TBN = float3x3(flatRight, fixedUp, N);

    return output;
}
///EOF VSParticle_Cylindrical.hlsl