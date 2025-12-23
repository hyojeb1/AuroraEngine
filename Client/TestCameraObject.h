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

private:
	void InitializeGameObject() override;
	void UpdateGameObject(float deltaTime) override;
};