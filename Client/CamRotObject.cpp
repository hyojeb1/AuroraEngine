#include "stdafx.h"
#include "CamRotObject.h"

#include "InputManager.h"
#include "TimeManager.h"
#include "CameraComponent.h"

REGISTER_TYPE(CamRotObject)

using namespace DirectX;

void CamRotObject::Initialize()
{
	GetComponent<CameraComponent>()->SetAsMainCamera();
}

void CamRotObject::Update()
{
	InputManager& input = InputManager::GetInstance();

	float pitch = XMVectorGetX(GetRotation()) + static_cast<float>(input.GetMouseDelta().y) * m_ySensitivity;

	constexpr float LIMIT = 90.0f - 1.0f;
	if (pitch > LIMIT) pitch = LIMIT;
	if (pitch < -LIMIT) pitch = -LIMIT;

	SetRotation(XMVectorSet(pitch, 0.0f, 0.0f, 0.0f));
}