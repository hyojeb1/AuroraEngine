#include "stdafx.h"
#include "TitleScene.h"

#include "SceneManager.h"
#include "CameraComponent.h"

#include "UIBase.h"
#include "SoundManager.h"

REGISTER_TYPE(TitleScene);

void TitleScene::Initialize()
{
	GetRootGameObject("MainCam")->GetComponent<class CameraComponent>()->SetAsMainCamera();

	float buttonX = 0.85f;

	Button* startButton = CreateUI<Button>();
	startButton->SetTextureAndOffset("IDLE.png", "HOVERD.png", "PRESSED.png", "CLICKED.png");
	startButton->SetLocalPosition({ buttonX, 0.3f });
	startButton->SetOnClick([]() { SceneManager::GetInstance().ChangeScene("TestScene"); });
	startButton->SetScale(0.3f);

	Button* optionButton = CreateUI<Button>();
	optionButton->SetTextureAndOffset("IDLE.png", "HOVERD.png", "PRESSED.png", "CLICKED.png");
	optionButton->SetLocalPosition({ buttonX, 0.5f });
	optionButton->SetScale(0.3f);

	Button* creditbutton = CreateUI<Button>();
	creditbutton->SetTextureAndOffset("IDLE.png", "HOVERD.png", "PRESSED.png", "CLICKED.png");
	creditbutton->SetLocalPosition({ buttonX, 0.7f });
	creditbutton->SetScale(0.3f);

	Button* exitbutton = CreateUI<Button>();
	exitbutton->SetTextureAndOffset("IDLE.png", "HOVERD.png", "PRESSED.png", "CLICKED.png");
	exitbutton->SetLocalPosition({ buttonX, 0.9f });
	exitbutton->SetScale(0.3f);

	SoundManager::GetInstance().Ambience_Shot(Config::Ambience);
}

void TitleScene::Update()
{
}