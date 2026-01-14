#include "stdafx.h"
#include "TestCameraObject.h"
#include "CameraComponent.h"

REGISTER_TYPE(TestCameraObject)

void TestCameraObject::Initialize()
{
	SetPosition({ 0.0f, 10.0f, -20.0f });
	LookAt({ 0.0f, 0.0f, 0.0f });
	CreateComponent<CameraComponent>();
}

void TestCameraObject::Update()
{
}