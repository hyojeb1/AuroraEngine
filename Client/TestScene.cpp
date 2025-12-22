#include "stdafx.h"
#include "TestScene.h"

#include "TestObject.h"
#include "TestCameraObject.h"

using namespace std;

void TestScene::InitializeScene()
{
	CreateRootGameObject<TestObject>()->CreateChildGameObject<TestObject>()->SetPosition({ 2.0f, 0.0f, 0.0f });
}

GameObjectBase* TestScene::CreateCameraObject()
{
	return CreateRootGameObject<TestCameraObject>();
}