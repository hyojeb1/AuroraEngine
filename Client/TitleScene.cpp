#include "stdafx.h"
#include "TitleScene.h"

#include "SceneManager.h"
#include "CameraComponent.h"
#include "SoundManager.h"
#include "TimeManager.h"

#include "UIBase.h"
#include "Button.h"
#include "Panel.h"
#include "Slider.h"
#include "Text.h"


#include "Shared/Config/Option.h"

REGISTER_TYPE(TitleScene);

void TitleScene::Initialize()
{
	GetRootGameObject("MainCam")->GetComponent<class CameraComponent>()->SetAsMainCamera();

	if (optionPanel) optionPanel->SetActive(false);
	if (creditPanel) creditPanel->SetActive(false);
	if (Titles) Titles->SetActive(true);
	if (Title_letterrbox_down) Title_letterrbox_down->SetActive(true);
	if (Title_letterrbox_up) Title_letterrbox_up->SetActive(true);


	//float buttonX = 0.85f;

	//Panel* logo = CreateUI<Panel>();
	//logo->SetTextureAndOffset("UI_logo.png");
	//logo->SetLocalPosition({ 0.5f, 0.3f });
	//logo->SetScale(0.5f);

	//Panel* OptionPanel = CreateUI<Panel>();
	//OptionPanel->SetTextureAndOffset("UI_Panel.png");
	//OptionPanel->SetLocalPosition({ 0.5f, 0.5f });
	//OptionPanel->SetScale(1);

	//Button* OptionClose = CreateUI<Button>();
	//OptionClose->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Clicked.png");
	//OptionClose->SetParent(OptionPanel);
	//OptionClose->SetLocalPosition({ 0.0f, 0.0f });
	//OptionClose->SetOnClick([OptionPanel]() { OptionPanel->SetActive(false); });
	//OptionClose->SetScale(0.3f);

	//Button* startButton = CreateUI<Button>();
	//startButton->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Clicked.png");
	//startButton->SetLocalPosition({ buttonX, 0.3f });
	//startButton->SetOnClick([]() { SceneManager::GetInstance().ChangeScene("TaehyeonTestScene"); });	//fade in 로직 추가 및 fade 종료시 Scene 종료로 변경
	//startButton->SetScale(0.3f);

	//Button* optionButton = CreateUI<Button>();
	//optionButton->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Clicked.png");
	//optionButton->SetLocalPosition({ buttonX, 0.5f });
	//optionButton->SetOnClick([OptionPanel]() { OptionPanel->SetActive(true); });
	//optionButton->SetScale(0.3f);

	//Button* creditbutton = CreateUI<Button>();
	//creditbutton->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Clicked.png");
	//creditbutton->SetLocalPosition({ buttonX, 0.7f });
	//creditbutton->SetScale(0.3f);

	//Button* exitbutton = CreateUI<Button>();
	//exitbutton->SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Clicked.png");
	//exitbutton->SetLocalPosition({ buttonX, 0.9f });
	//exitbutton->SetScale(0.3f);

	//SoundManager::GetInstance().Ambience_Shot(Config::Ambience);
}

void TitleScene::Update()
{

}

void TitleScene::BindUIActions()
{
	for (const auto& uiPtr : m_UIList) {
		if (auto* panel = dynamic_cast<Panel*>(uiPtr.get())) {
			if (panel->GetName() == "option") optionPanel = panel;
			else if (panel->GetName() == "UI_Title_letterrbox_down") Title_letterrbox_down = panel;
			else if (panel->GetName() == "UI_Title_letterrbox_up") Title_letterrbox_up = panel;
			else if (panel->GetName() == "credit") creditPanel = panel;
			else if (panel->GetName() == "Titles") Titles = panel;
		} else if (auto* text = dynamic_cast<Text*>(uiPtr.get())) {
			//if (text->GetName() == "result_time") resultTime = text;
		}
	}


	for (auto& uiPtr : m_UIList) {
		// -------------------------------------------------------
		// 1. Button bindings
		// -------------------------------------------------------
		if (auto* btn = dynamic_cast<Button*>(uiPtr.get())) {
			std::string key = btn->GetActionKey();

			if (key == "start_game") {
				btn->SetOnClick([this]() { SceneManager::GetInstance().ChangeScene("TestScene");  });
			} else if (key == "quit_game") {
				btn->SetOnClick([]() { PostQuitMessage(0); });
			} else if (key == "open_option") {
				if (optionPanel) btn->SetOnClick([this]() { optionPanel->SetActive(true); });
			} else if (key == "close_option") {
				if (optionPanel) btn->SetOnClick([this]() { optionPanel->SetActive(false); });
			} else if (key == "open_credit") {
				if (optionPanel) btn->SetOnClick([this]() { creditPanel->SetActive(true); });
			} else if (key == "close_credit") {
				if (optionPanel) btn->SetOnClick([this]() { creditPanel->SetActive(false); });
			}
		}

		// -------------------------------------------------------
		// 2. Slider bindings
		// -------------------------------------------------------
		else if (auto* slider = dynamic_cast<Slider*>(uiPtr.get())) {
			std::string key = slider->GetActionKey();

			if (key == "MasterVolume") {
				slider->AddListener([](float val) {
					SoundManager::GetInstance().SetVolume_Main(val);
					});
				slider->NotifyValueChanged();
			} else if (key == "Gamma") {
				slider->AddListener([](float val) {
					SceneBase::SetGammaIntensity(val);
					});
				slider->NotifyValueChanged();
			}
		}
	}
}


void TitleScene::MovingPanel(float dt)
{

}
