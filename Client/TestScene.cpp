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
}

/// TestScene.cpp?ùò ?Åù
