/// TestScene.cpp의 시작
#include "stdafx.h"
#include "TestScene.h"

#include "TestObject.h"
#include "TestCameraObject.h"
#include "CamRotObject.h"
#include "TimeManager.h"

REGISTER_TYPE(TestScene)

using namespace std;
using namespace DirectX;

void TestScene::Initialize()
{
	ShowCursor(FALSE);
}

void TestScene::Update()
{
	static float time = 0.0f;
	time += TimeManager::GetInstance().GetDeltaTime();

	if (time > 1.0f)
	{
		time = 0.0f;

		float x = static_cast<float>(rand() % 21 - 10);
		float z = static_cast<float>(rand() % 21 - 10);

		CreateRootGameObject("Enemy")->SetPosition(XMVectorSet(x, 0.0f, z, 1.0f));
	}
}

/// TestScene.cpp의 끝