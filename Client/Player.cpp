#include "stdafx.h"
#include "Player.h"

#include "TimeManager.h"
#include "InputManager.h"
#include "ColliderComponent.h"
#include "CameraComponent.h"

REGISTER_TYPE(Player)

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
	if (input.GetKey(A)) MoveDirection(deltaTime * 5.0f, Direction::Left);
	if (input.GetKey(D)) MoveDirection(deltaTime * 5.0f, Direction::Right);
	if (input.GetKey(Space)) MoveDirection(deltaTime * 5.0f, Direction::Up);
	if (input.GetKey(Shift)) MoveDirection(deltaTime * 5.0f, Direction::Down);

	if (input.GetKeyDown(MouseLeft))
	{
		float distance = 0.0f;
		GameObjectBase* hit = ColliderComponent::CheckCollision(g_mainCamera->GetPosition(), g_mainCamera->GetForwardVector(), distance);
		if (hit) hit->SetAlive(false);
	}
}