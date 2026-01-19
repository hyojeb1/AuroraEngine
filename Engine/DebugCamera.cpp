// BOF DebugCamera.cpp
#include "stdafx.h"
#include "DebugCamera.h"

#include "InputManager.h"
#include "TimeManager.h"

using namespace DirectX;

void DebugCamera::Initialize()
{
	m_targetPosition = { 0.0f, 0.0f, 0.0f };
	m_distance = 10.0f;
	m_pitch = 0.2f; 
	m_yaw = 0.0f;
}

void DebugCamera::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();
    InputManager& input = InputManager::GetInstance();
	MousePos delta = input.GetMouseDelta();
	int wheel = input.GetMouseWheel();
	//bool isMouseLeft = input.GetKey(KeyCode::MouseLeft); //이건 피킹 구현할 때 개발할 예정
	bool isMouseRight = input.GetKey(KeyCode::MouseRight);
	bool isMouseMiddle = input.GetKey(KeyCode::MouseMiddle);
	
	XMVECTOR current_position = GetPosition();

	// -----------------------------------------------------------
	// [기능 1] 줌 (Zoom) - 거리 조절
	// -----------------------------------------------------------
	if (wheel != 0)
	{
		float zoomSpeed = m_distance * 0.1f;
		if (zoomSpeed < 0.5f) zoomSpeed = 0.5f; 
		m_distance -= static_cast<float>(wheel) / 120.0f * m_wheelSensitivity * zoomSpeed;

		if (m_distance < 0.1f) m_distance = 0.1f;
	}

	// -----------------------------------------------------------
	// [기능 2] 오빗 (Orbit) - 마우스 휠 클릭 중일 때
	// -----------------------------------------------------------
	if (isMouseMiddle)
	{
		// === [Orbit] 그냥 드래그 : 회전 ===
		// 마우스 좌우 -> Yaw(Y축 회전), 상하 -> Pitch(X축 회전)
		m_yaw += static_cast<float>(delta.x) * m_orbitSensitivity;
		m_pitch += static_cast<float>(delta.y) * m_orbitSensitivity;

		// 고개를 너무 젖혀서 뒤집히지 않게 제한 (짐벌락 방지)
		// -89도 ~ 89도 사이로 제한 (라디안 변환)
		constexpr float LIMIT = XM_PI / 2.0f - 0.01f;
		if (m_pitch > LIMIT) m_pitch = LIMIT;
		if (m_pitch < -LIMIT) m_pitch = -LIMIT;
	}


	// -----------------------------------------------------------
	// [기능 3] 팬(Pan)
	// -----------------------------------------------------------
	if (isMouseRight)
	{
		XMVECTOR camRight, camUp, camForward;
		GetCameraBasis(camRight, camUp, camForward);

		float panSpeed = m_distance * m_panSensitivity * 0.1f;

		XMVECTOR moveVector = (camRight * static_cast<float>(-delta.x) * panSpeed) + (camUp * static_cast<float>(delta.y) * panSpeed);

		XMVECTOR currentTarget = XMLoadFloat3(&m_targetPosition);
		currentTarget += moveVector;
		XMStoreFloat3(&m_targetPosition, currentTarget);
	}

	// -----------------------------------------------------------
	// [기능 4] 최종 위치 계산 (Spherical Coordinates)
	// -----------------------------------------------------------
	// 최종 위치 = 타겟 위치 - (바라보는방향 * 거리)
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
	XMVECTOR lookDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	lookDir = XMVector3TransformNormal(lookDir, rotationMatrix);
	XMVECTOR targetPos = XMLoadFloat3(&m_targetPosition);
	XMVECTOR finalPos = targetPos - (lookDir * m_distance);

	SetPosition(finalPos);
	LookAt(targetPos);
}

//이건 피킹 구현할 때 개발할 예정
void DebugCamera::FocusTarget(const DirectX::XMVECTOR& targetPos, float distance)
{
	XMStoreFloat3(&m_targetPosition, targetPos);
	m_distance = distance;

	// 포커싱 할 때 적당한 각도로 초기화하고 싶다면 주석 해제
	// m_pitch = 0.0f;
	// m_yaw = 0.0f;
}

void DebugCamera::GetCameraBasis(DirectX::XMVECTOR& outRight, DirectX::XMVECTOR& outUp, DirectX::XMVECTOR& outForward)
{
	// 현재 Pitch/Yaw를 기반으로 회전 행렬 생성
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);

	// 행렬의 행(Row) 또는 열(Col)이 기저 벡터임 (DirectX Math는 보통 Row-Major)
	// 1행: Right, 2행: Up, 3행: Forward
	outRight = rotationMatrix.r[0];
	outUp = rotationMatrix.r[1];
	outForward = rotationMatrix.r[2];
}

// EOF DebugCamera.cpp