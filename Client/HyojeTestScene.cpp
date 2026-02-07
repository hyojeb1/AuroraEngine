///bof  HyojeTestScene.cpp
#include "stdafx.h"
#include "HyojeTestScene.h"

#include "InputManager.h"
#include "SceneManager.h"
#include "TimeManager.h"
#include "SoundManager.h"

#include "TestCameraObject.h"
#include "CamRotObject.h"

#include "Enemy.h"
#include "RNG.h"

#include "Shared/Config/Option.h"

#include "Button.h"
#include "Panel.h"
#include "Slider.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(HyojeTestScene)

void HyojeTestScene::Initialize()
{
	ShowCursor(FALSE);

	SoundManager::GetInstance().Main_BGM_Shot(Config::Main_BGM, 1.0f);
	SoundManager::GetInstance().Ambience_Shot(Config::Ambience);


}


void HyojeTestScene::Update()
{
	static float time = 0.0f;
	time += TimeManager::GetInstance().GetDeltaTime();

	if (time > 1.0f) {
		time = 0.0f;

		constexpr float SPREAD = 10.0f;

		CreatePrefabRootGameObject("Enemy.json")->SetPosition(XMVectorSet(RNG::GetInstance().Range(-SPREAD, SPREAD), 0.0f, RNG::GetInstance().Range(-SPREAD, SPREAD), 1.0f));
	}

	if (InputManager::GetInstance().GetKeyDown(KeyCode::Num0)) {
		SceneManager::GetInstance().ChangeScene("EndingScene");
	}

	if (SoundManager::GetInstance().CheckBGMEnd()) {
		SoundManager::GetInstance().Main_BGM_Shot(SoundManager::GetInstance().GetCurrentTrackName(), 3.0f);
	}


}

void HyojeTestScene::Finalize() {
	SoundManager::GetInstance().Stop_ChannelGroup();
}


void  HyojeTestScene::BindUIActions()
{
    Panel* optionPanel = nullptr;
    for (const auto& uiPtr : m_UIList) {
        if (auto* panel = dynamic_cast<Panel*>(uiPtr.get())) {
            if (panel->GetName() == "OptionPanel") {
                optionPanel = panel;
                break;
            }
        }
    }

    for (auto& uiPtr : m_UIList) {
        // -------------------------------------------------------
        // 1. Button bindings
        // -------------------------------------------------------
        if (auto* btn = dynamic_cast<Button*>(uiPtr.get())) {
            std::string key = btn->GetActionKey();

            if (key == "StartGame") {
                btn->SetOnClick([]() { SceneManager::GetInstance().ChangeScene("TaehyeonTestScene"); });
            } else if (key == "QuitGame") {
                btn->SetOnClick([]() { PostQuitMessage(0); });
            } else if (key == "OpenOption") {
                if (optionPanel) btn->SetOnClick([optionPanel]() { optionPanel->SetActive(true); });
            } else if (key == "CloseOption") {
                if (optionPanel) btn->SetOnClick([optionPanel]() { optionPanel->SetActive(false); });
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
