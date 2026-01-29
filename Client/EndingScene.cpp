
#include "stdafx.h"
#include "EndingScene.h"

#include "SceneManager.h"
#include "TimeManager.h"
#include "SoundManager.h"

#include "CameraComponent.h"

REGISTER_TYPE(EndingScene)

using namespace std;
using namespace DirectX;

void EndingScene::Initialize()
{
	GetRootGameObject("MainCam")->GetComponent<class CameraComponent>()->SetAsMainCamera();

	ShowCursor(TRUE);

	Button* TitleButton = CreateButton();
	TitleButton->SetTextureAndOffset("IDLE.png", "HOVERD.png", "PRESSED.png", "CLICKED.png");
	TitleButton->SetUIPosition({ 0.5f, 0.5f });
	TitleButton->SetOnClick([]() { SceneManager::GetInstance().ChangeScene("TitleScene"); });
	TitleButton->SetScale(0.3f);
}

void EndingScene::Update()
{
}