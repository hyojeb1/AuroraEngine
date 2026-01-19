///BOF TestCameraObject.cpp
#include "stdafx.h"
#include "TestCameraObject.h"
#include "CameraComponent.h"
#include "InputManager.h"
#include "TimeManager.h"

using namespace DirectX;

REGISTER_TYPE(TestCameraObject)

void TestCameraObject::Initialize()
{
	m_yaw = 0.0f;
	m_pitch = 0.0f;
}

void TestCameraObject::Update()
{
    float deltaTime = TimeManager::GetInstance().GetDeltaTime();
    InputManager& input = InputManager::GetInstance();

    // 마우스 우클릭 상태일 때만 FPS 모드 활성화 (일반적인 엔진 조작 방식)
    if (input.GetKey(KeyCode::MouseRight))
    {
        // ===========================================================
        // [기능 0] 마우스를 게임 화면의 중심으로 보냄
        // ===========================================================
        //

        // ===========================================================
        // [기능 1] 마우스 회전 (Look)
        // ===========================================================
        POINT delta = input.GetMouseDelta();

        // 마우스 좌우 이동 -> Yaw (Y축 회전)
        m_yaw += static_cast<float>(delta.x) * m_lookSensitivity;

        // 마우스 상하 이동 -> Pitch (X축 회전)
        // FPS 게임은 마우스를 올리면 화면이 위로 가야 하므로 보통 -delta.y를 하거나
        // 엔진 좌표계에 따라 부호를 맞춥니다. (여기서는 일반적인 FPS 기준)
        m_pitch += static_cast<float>(delta.y) * m_lookSensitivity;

        // 짐벌락 방지 및 고개 꺾임 방지 (-89도 ~ 89도 제한)
        constexpr float LIMIT = XM_PI / 2.0f - 0.01f;
        if (m_pitch > LIMIT) m_pitch = LIMIT;
        if (m_pitch < -LIMIT) m_pitch = -LIMIT;

        // ===========================================================
        // [기능 2] 키보드 이동 (WASD + Q/E)
        // ===========================================================

        // 현재 회전(Yaw/Pitch)을 기준으로 방향 벡터를 가져옴
        XMVECTOR camRight, camUp, camForward;
        GetCameraBasis(camRight, camUp, camForward);

        //XMVECTOR moveDir = XMVectorSet(0.f, 0.f, 0.f, 0.f);

        //// W, S : 앞뒤 이동 (바라보는 방향 기준)
        //if (input.GetKey(KeyCode::W)) moveDir += camForward;
        //if (input.GetKey(KeyCode::S)) moveDir -= camForward;

        //// A, D : 좌우 이동 (오른쪽 벡터 기준)
        //if (input.GetKey(KeyCode::D)) moveDir += camRight;
        //if (input.GetKey(KeyCode::A)) moveDir -= camRight;

        //// Q, E : 수직 상승/하강 (옵션, Global Y축 혹은 로컬 Up)
        //// 비행 모드처럼 하려면 Global Up(0,1,0)을 쓰는게 편합니다.
        //if (input.GetKey(KeyCode::E)) moveDir += XMVectorSet(0.f, 1.f, 0.f, 0.f); // 위로
        //if (input.GetKey(KeyCode::Q)) moveDir -= XMVectorSet(0.f, 1.f, 0.f, 0.f); // 아래로

        //// 벡터 정규화 (대각선 이동 시 속도 빨라짐 방지)
        //moveDir = XMVector3Normalize(moveDir);

        //// 최종 위치 적용
        //XMVECTOR currentPos = GetPosition();
        //XMVECTOR velocity = moveDir * m_moveSpeed * deltaTime;

        //// Shift 키를 누르면 부스트 (속도 2배) - 디버그 시 유용
        //if (input.GetKey(KeyCode::Shift)) velocity *= 2.0f;

        //SetPosition(currentPos + velocity);
    }

    // ===========================================================
    // [기능 3] 뷰 행렬 갱신 (LookAt 계산)
    // ===========================================================
    // FPS 카메라는 "내 위치"에서 "바라보는 방향"을 더한 곳을 봅니다.

    // 1. 회전 행렬 생성
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);

    // 2. 기본 전방 벡터(0,0,1)를 회전시켜 실제 바라보는 방향(LookDir) 구하기
    XMVECTOR worldForward = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);

    // 3. 타겟 지점 = 내 위치 + 바라보는 방향
    XMVECTOR currentPos = GetPosition();
    XMVECTOR targetPos = currentPos + worldForward;

    // 4. LookAt 적용
    LookAt(targetPos);
}

void TestCameraObject::GetCameraBasis(DirectX::XMVECTOR& outRight, DirectX::XMVECTOR& outUp, DirectX::XMVECTOR& outForward)
{
	XMMATRIX rotationMatrix			= XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
	outRight						= rotationMatrix.r[0];
	outUp							= rotationMatrix.r[1];
	outForward						= rotationMatrix.r[2];
}
///EOF TestCameraObject.cpp