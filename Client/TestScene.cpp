#include "stdafx.h"
#include "TestScene.h"

#include "InputManager.h"
#include "SceneManager.h"
#include "TimeManager.h"
#include "SoundManager.h"

#include "TestCameraObject.h"
#include "CamRotObject.h"

#include "Enemy.h"
#include "RNG.h"

REGISTER_TYPE(TestScene)

using namespace std;
using namespace DirectX;

void TestScene::Initialize()
{
	ShowCursor(FALSE);

	SoundManager::GetInstance().Main_BGM_Shot(Config::Main_BGM,1.0f);
	SoundManager::GetInstance().Ambience_Shot(Config::Ambience);
}

void TestScene::Update()
{
	static float time = 0.0f;
	time += TimeManager::GetInstance().GetDeltaTime();

	if (time > 1.0f)
	{
		time = 0.0f;

		constexpr float SPREAD = 10.0f;

		CreatePrefabRootGameObject("Enemy.json")->SetPosition(XMVectorSet(RNG::GetInstance().Range(-SPREAD, SPREAD), 0.0f, RNG::GetInstance().Range(-SPREAD, SPREAD), 1.0f));
	}

	if (InputManager::GetInstance().GetKeyDown(KeyCode::Num0))
	{
		SceneManager::GetInstance().ChangeScene("EndingScene");
	}

	if (SoundManager::GetInstance().CheckBGMEnd())
	{
		SoundManager::GetInstance().Main_BGM_Shot(SoundManager::GetInstance().GetCurrentTrackName(), 3.0f);
	}
}

void TestScene::Finalize()
{
	SoundManager::GetInstance().Stop_ChannelGroup();
}
