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
}

void HyojeTestScene::Update()
{
	static float time = 0.0f;
	time += TimeManager::GetInstance().GetDeltaTime();

}
