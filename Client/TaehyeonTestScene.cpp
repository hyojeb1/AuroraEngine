/// TaehyeonTestScene.cpp의 시작
#include "stdafx.h"
#include "TaehyeonTestScene.h"

#include "TestCameraObject.h"
#include "CamRotObject.h"
#include "TimeManager.h"
#include "SoundManager.h"

#include "Shared/Config/Option.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(TaehyeonTestScene)

void TaehyeonTestScene::Initialize()
{
	ShowCursor(FALSE);

	SoundManager::GetInstance().Main_BGM_Shot(Config::Main_BGM, 3.0f);
	SoundManager::GetInstance().Ambience_Shot(Config::Ambience);
}

void TaehyeonTestScene::Update()
{
	static float time = 0.0f;
	time += TimeManager::GetInstance().GetDeltaTime();

}
