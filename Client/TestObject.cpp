#include "stdafx.h"
#include "TestObject.h"

#include "ModelComponent.h"

using namespace DirectX;

void TestObject::Begin()
{
	AddComponent<ModelComponent>();
}

void TestObject::Update(float deltaTime)
{
	Rotate({ 0.0f, deltaTime, 0.0f, 0.0f });
}