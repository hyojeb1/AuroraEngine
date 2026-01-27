/// TaehyeonTestScene.cpp의 시작
#include "stdafx.h"
#include "TaehyeonTestScene.h"

#include "TestCameraObject.h"
#include "CamRotObject.h"
#include "TimeManager.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(TaehyeonTestScene)

void TaehyeonTestScene::Initialize()
{
}

void TaehyeonTestScene::Update()
{
	static float time = 0.0f;
	time += TimeManager::GetInstance().GetDeltaTime();

}
