#include "stdafx.h"
#include "TitleScene.h"

#include "SceneManager.h"
#include "CameraComponent.h"

REGISTER_TYPE(TitleScene);

void TitleScene::Initialize()
{
	GetRootGameObject("MainCam")->GetComponent<class CameraComponent>()->SetAsMainCamera();

	Button* startButton = CreateButton();
	startButton->SetTextureAndOffset("Crosshair.png");
	startButton->SetUIPosition({ 0.75f, 0.5f });
	startButton->SetOnClick([&]() { SceneManager::GetInstance().ChangeScene("HyojeTestScene"); });
}

void TitleScene::Update()
{
}