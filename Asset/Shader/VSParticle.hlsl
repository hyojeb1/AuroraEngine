/// VSParticle.hlsl의 시작
#include "CommonVS.hlsli"

VS_OUTPUT_STD main(VS_INPUT_POS_UV input)
{
    VS_OUTPUT_STD output;

    // 1. 파티클의 중심 위치 계산 (WorldMatrix의 Translation 부분만 가져옴)
    // 일반적인 WorldMatrix 곱셈을 하면 파티클도 같이 회전해버리므로, 위치만 따옵니다.
    float3 centerWorldPos = float3(WorldMatrix._41, WorldMatrix._42, WorldMatrix._43);

    // 2. 카메라의 기저 벡터(Right, Up) 추출
    // 뷰 행렬(World -> View)의 역행렬(View -> World)은 카메라의 월드 정보를 담고 있습니다.
    // 뷰 행렬이 직교 행렬이라면, 전치 행렬이 곧 역행렬입니다.
    // HLSL의 행렬 저장 방식(Row-major) 기준:
    // ViewMatrix의 1열 = Camera Right Vector
    // ViewMatrix의 2열 = Camera Up Vector
    float3 cameraRight = float3(ViewMatrix._11, ViewMatrix._21, ViewMatrix._31);
    float3 cameraUp = float3(ViewMatrix._12, ViewMatrix._22, ViewMatrix._32);
    // float3 cameraLook  = float3(ViewMatrix._13, ViewMatrix._23, ViewMatrix._33); // 필요하면 사용

    // 3. 빌보딩 적용 (중심점에서 카메라 축 방향으로 버텍스 확장)
    // input.Position.x와 y는 중심점으로부터의 오프셋(-0.5 ~ 0.5) 역할을 합니다.
    // 크기는 WorldMatrix의 스케일에서 가져오거나, 별도 상수로 곱해줄 수 있습니다.
    
    // 팁: WorldMatrix의 스케일(Scale)을 살리고 싶다면 아래와 같이 길이(length)를 구해서 곱해줍니다.
    float scaleX = length(float3(WorldMatrix._11, WorldMatrix._12, WorldMatrix._13));
    float scaleY = length(float3(WorldMatrix._21, WorldMatrix._22, WorldMatrix._23));

    float3 positionOffset = (input.Position.x * scaleX * cameraRight) +
                            (input.Position.y * scaleY * cameraUp);

    output.WorldPosition = float4(centerWorldPos + positionOffset, 1.0f);

    // 4. 투영 변환
    output.Position = mul(output.WorldPosition, VPMatrix);

    // 5. UV 전달
    output.UV = input.UV;

    // 6. 노말 처리 (TBN)
    // 파티클은 항상 카메라를 바라보므로, 노말은 -Look (카메라 쪽) 이어야 합니다.
    // 하지만 PBR 셰이딩을 그대로 쓴다면, 그냥 평평한 면의 노말을 만들어줍니다.
    float3 N = normalize(cross(cameraUp, cameraRight)); // == -CameraLook
    // 탄젠트, 바이탄젠트는 UV 방향과 일치시킵니다.
    output.TBN = float3x3(cameraRight, cameraUp, N);

    return output;
}
/// VSParticle.hlsl의 끝