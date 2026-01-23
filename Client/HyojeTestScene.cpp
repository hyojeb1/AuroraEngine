/// HyojeTestScene.cpp의 시작
#include "stdafx.h"
#include "HyojeTestScene.h"

#include "TestCameraObject.h"
#include "CamRotObject.h"
#include "TimeManager.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(HyojeTestScene)

void HyojeTestScene::Initialize()
{
	ShowCursor(FALSE);
}

void HyojeTestScene::Update()
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
