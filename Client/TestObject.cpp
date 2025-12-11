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
}