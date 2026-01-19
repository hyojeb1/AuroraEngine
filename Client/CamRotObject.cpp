///BOF CamRotObject.cpp
#include "stdafx.h"
#include "CamRotObject.h"

#include "InputManager.h"
#include "TimeManager.h"

REGISTER_TYPE(CamRotObject)

void CamRotObject::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();

	using enum KeyCode;
	auto& input = InputManager::GetInstance();

	if (input.GetKey(Left)) Rotate({ 0.0f, deltaTime * 45.0f, 0.0f });
	if (input.GetKey(Right)) Rotate({ 0.0f, -deltaTime * 45.0f, 0.0f });
	if (input.GetKey(Up)) Rotate({ deltaTime * 45.0f, 0.0f, 0.0f });
	if (input.GetKey(Down)) Rotate({ -deltaTime * 45.0f, 0.0f, 0.0f });

	if (input.GetKey(A)) MoveDirection(deltaTime * 10.0f, Direction::Left);
	if (input.GetKey(D)) MoveDirection(deltaTime * 10.0f, Direction::Right);
	if (input.GetKey(W)) MoveDirection(deltaTime * 10.0f, Direction::Forward);
	if (input.GetKey(S)) MoveDirection(deltaTime * 10.0f, Direction::Backward);
	if (input.GetKey(Space)) MoveDirection(deltaTime * 10.0f, Direction::Up);
	if (input.GetKey(Shift)) MoveDirection(deltaTime * 10.0f, Direction::Down);
}

///EOF CamRotObject.cpp