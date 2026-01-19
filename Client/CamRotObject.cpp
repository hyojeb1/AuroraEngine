///BOF CamRotObject.cpp
#include "stdafx.h"
#include "CamRotObject.h"

#include "InputManager.h"
#include "TimeManager.h"

REGISTER_TYPE(CamRotObject)

using namespace DirectX;

void CamRotObject::Initialize()
{
	auto& input = InputManager::GetInstance();
}

void CamRotObject::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();
	auto& input = InputManager::GetInstance();

	const POINT& delta = input.GetMouseDelta();
	const XMVECTOR& euler = GetRotation();

	float yaw = XMVectorGetY(euler) + static_cast<float>(delta.x) * 0.1f;
	float pitch = XMVectorGetX(euler) + static_cast<float>(delta.y) * 0.1f;

	constexpr float LIMIT = 90.0f - 1.0f;
	if (pitch > LIMIT) pitch = LIMIT;
	if (pitch < -LIMIT) pitch = -LIMIT;

	SetRotation(XMVectorSet(pitch, yaw, 0.0f, 0.0f));
}

///EOF CamRotObject.cpp