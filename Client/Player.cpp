#include "stdafx.h"
#include "Player.h"

#include "TimeManager.h"
#include "InputManager.h"

void Player::Initialize()
{
}

void Player::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();

	using enum KeyCode;
	auto& input = InputManager::GetInstance();

	if (input.GetKey(W)) MoveDirection(deltaTime * 5.0f, Direction::Forward);
	if (input.GetKey(S)) MoveDirection(deltaTime * 5.0f, Direction::Backward);
	if (input.GetKey(Space)) MoveDirection(deltaTime * 5.0f, Direction::Up);
	if (input.GetKey(Shift)) MoveDirection(deltaTime * 5.0f, Direction::Down);
	if (input.GetKey(A)) Rotate({ 0.0f, -deltaTime * 90.0f, 0.0f });
	if (input.GetKey(D)) Rotate({ 0.0f, deltaTime * 90.0f, 0.0f });
}