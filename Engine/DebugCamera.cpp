// BOF DebugCamera.cpp
#include "stdafx.h"
#include "DebugCamera.h"

#include "InputManager.h"
#include "TimeManager.h"
#include "CameraComponent.h"

using namespace DirectX;

void DebugCamera::Initialize()
{
	CreateComponent<CameraComponent>()->SetAsMainCamera();

	SetPosition(XMVectorSet(10.0f, 10.0f, -10.0f, 0.0f));
	LookAt(XMVectorZero());
}

void DebugCamera::Update()
{
	InputManager& input = InputManager::GetInstance();
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();
	const POINT& mouseDelta = input.GetMouseDelta();

	if (input.GetKey(KeyCode::MouseRight))
	{
		const XMVECTOR& euler = GetRotation();

		float yaw = XMVectorGetY(euler) + static_cast<float>(mouseDelta.x) * m_sensitivity;
		float pitch = XMVectorGetX(euler) + static_cast<float>(mouseDelta.y) * m_sensitivity;

		constexpr float LIMIT = 90.0f - 1.0f;
		if (pitch > LIMIT) pitch = LIMIT;
		if (pitch < -LIMIT) pitch = -LIMIT;

		SetRotation(XMVectorSet(pitch, yaw, 0.0f, 0.0f));
	}

	if (input.GetKey(KeyCode::W)) MovePosition(GetWorldDirectionVector(Direction::Forward) * m_moveSpeed * deltaTime);
	if (input.GetKey(KeyCode::S)) MovePosition(GetWorldDirectionVector(Direction::Backward) * m_moveSpeed * deltaTime);
	if (input.GetKey(KeyCode::A)) MovePosition(GetWorldDirectionVector(Direction::Left) * m_moveSpeed * deltaTime);
	if (input.GetKey(KeyCode::D)) MovePosition(GetWorldDirectionVector(Direction::Right) * m_moveSpeed * deltaTime);
	if (input.GetKey(KeyCode::Space)) MovePosition(GetWorldDirectionVector(Direction::Up) * m_moveSpeed * deltaTime);
	if (input.GetKey(KeyCode::Shift)) MovePosition(GetWorldDirectionVector(Direction::Down) * m_moveSpeed * deltaTime);
}

#ifdef _DEBUG
void DebugCamera::RenderImGui()
{
	ImGui::Begin("Debug Camera Settings");
	ImGui::SliderFloat("Sensitivity", &m_sensitivity, 0.01f, 1.0f);
	ImGui::SliderFloat("Move Speed", &m_moveSpeed, 1.0f, 100.0f);
	ImGui::End();
}
#endif