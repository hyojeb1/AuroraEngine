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
	auto& input = InputManager::GetInstance();

	if (input.GetKey(KeyCode::W)) MoveDirection(deltaTime * 5.0f, Direction::Forward);
	if (input.GetKey(KeyCode::S)) MoveDirection(deltaTime * 5.0f, Direction::Backward);
	if (input.GetKey(KeyCode::A)) MoveDirection(deltaTime * 5.0f, Direction::Left);
	if (input.GetKey(KeyCode::D)) MoveDirection(deltaTime * 5.0f, Direction::Right);
	if (input.GetKey(KeyCode::Space)) MoveDirection(deltaTime * 5.0f, Direction::Up);
	if (input.GetKey(KeyCode::Shift)) MoveDirection(deltaTime * 5.0f, Direction::Down);

	if (input.GetKeyDown(KeyCode::MouseLeft))
	{
		float distance = 0.0f;
		const CameraComponent& mainCamera = CameraComponent::GetMainCamera();
		GameObjectBase* hit = ColliderComponent::CheckCollision(mainCamera.GetPosition(), mainCamera.GetForwardVector(), distance);
		if (hit) hit->SetAlive(false);
	}
}