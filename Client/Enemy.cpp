#include "stdafx.h"
#include "Enemy.h"

#include "ModelComponent.h"
#include "ColliderComponent.h"

REGISTER_TYPE(Enemy)

using namespace DirectX;

void Enemy::Initialize()
{
	CreateComponent<ModelComponent>();
	CreateComponent<ColliderComponent>()->AddBoundingBox(BoundingBox({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }));
}