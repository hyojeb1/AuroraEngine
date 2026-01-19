///BOF TestCameraObject.h
#pragma once
#include "GameObjectBase.h"

class TestCameraObject : public GameObjectBase
{
public:
	TestCameraObject() = default;
	~TestCameraObject() override = default;
	TestCameraObject(const TestCameraObject&) = default;
	TestCameraObject& operator=(const TestCameraObject&) = default;
	TestCameraObject(TestCameraObject&&) = default;
	TestCameraObject& operator=(TestCameraObject&&) = default;

	// FPS 모드에서는 Forward 벡터가 중요하므로 유지
	void GetCameraBasis(DirectX::XMVECTOR& outRight, DirectX::XMVECTOR& outUp, DirectX::XMVECTOR& outForward);
private:
	void Initialize() override;
	void Update() override;

	// ROTATION: 회전 각도
	float m_pitch = 0.0f; // 상하 (X축 회전)
	float m_yaw = 0.0f;   // 좌우 (Y축 회전)

	// SETTINGS
	float m_lookSensitivity = 0.002f; // 마우스 회전 감도
	float m_moveSpeed = 10.0f;        // 키보드 이동 속도


};
///EOF TestCameraObject.h