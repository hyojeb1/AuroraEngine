#include "stdafx.h"
#include "TitleScene.h"

#include "SceneManager.h"
#include "CameraComponent.h"

#include "UIBase.h"
#include "Button.h"
#include "Panel.h"
#include "Slider.h"
#include "SoundManager.h"

#include "Shared/Config/Option.h"

REGISTER_TYPE(TitleScene);

void TitleScene::Initialize()
{
	GetRootGameObject("MainCam")->GetComponent<class CameraComponent>()->SetAsMainCamera();

	float buttonX = 0.85f;

	Panel* logo = CreateUI<Panel>();
	logo->SetTextureAndOffset("UI_logo.png");
	logo->SetLocalPosition({ 0.5f, 0.3f });
	logo->SetScale(0.5f);

	Panel* OptionPanel = CreateUI<Panel>();
	OptionPanel->SetTextureAndOffset("UI_Panel.png");
	OptionPanel->SetLocalPosition({ 0.5f, 0.5f });
	OptionPanel->SetScale(1);

	Button* OptionClose = CreateUI<Button>();
	OptionClose->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Cicked.png");
	OptionClose->SetParent(OptionPanel);
	OptionClose->SetLocalPosition({ 0.0f, 0.0f });
	OptionClose->SetOnClick([OptionPanel]() { OptionPanel->SetActive(false); });
	OptionClose->SetScale(0.3f);

	Button* startButton = CreateUI<Button>();
	startButton->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Cicked.png");
	startButton->SetLocalPosition({ buttonX, 0.3f });
	startButton->SetOnClick([]() { SceneManager::GetInstance().ChangeScene("TaehyeonTestScene"); });	//fade in 로직 추가 및 fade 종료시 Scene 종료로 변경
	startButton->SetScale(0.3f);

	Button* optionButton = CreateUI<Button>();
	optionButton->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Cicked.png");
	optionButton->SetLocalPosition({ buttonX, 0.5f });
	optionButton->SetOnClick([OptionPanel]() { OptionPanel->SetActive(true); });
	optionButton->SetScale(0.3f);

	Button* creditbutton = CreateUI<Button>();
	creditbutton->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Cicked.png");
	creditbutton->SetLocalPosition({ buttonX, 0.7f });
	creditbutton->SetScale(0.3f);

	Button* exitbutton = CreateUI<Button>();
	exitbutton->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Cicked.png");
	exitbutton->SetLocalPosition({ buttonX, 0.9f });
	exitbutton->SetScale(0.3f);

	SoundManager::GetInstance().Ambience_Shot(Config::Ambience);
}

void TitleScene::Update()
{
}