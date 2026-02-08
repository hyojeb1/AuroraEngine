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
}


void HyojeTestScene::Update()
{
    OnHyojeStateUpdate();
}

void HyojeTestScene::Finalize() 
{
}



void HyojeTestScene::ChangeState(EHyojeState newState)
{
    OnHyojeStateExit(m_currentState);
    m_currentState = newState;
    OnHyojeStateEnter(m_currentState);
}


void HyojeTestScene::OnHyojeStateEnter(EHyojeState type)
{
	switch (type) {
	case EHyojeState::Title:
		cout << "[HyojeTestScene] Title State Enter" << endl;
		ShowCursor(TRUE);
		break;

	case EHyojeState::Main:
		m_player_hp = kPlayerHP;
		cout << "[HyojeTestScene] Game Start! HP Reset" << endl;
		ShowCursor(FALSE);
		SoundManager::GetInstance().Main_BGM_Shot(Config::Main_BGM, 1.0f);
		SoundManager::GetInstance().Ambience_Shot(Config::Ambience);
		break;

	case EHyojeState::Result:
		cout << "[HyojeTestScene] Game Over / Result Show" << endl;
		ShowCursor(TRUE);
		break;
	}
}

void HyojeTestScene::OnHyojeStateUpdate()
{
	switch (m_currentState) {
	case EHyojeState::Title:
		if (InputManager::GetInstance().GetKeyDown(KeyCode::Space)) {
			ChangeState(EHyojeState::Main); 
		}
		break;

	case EHyojeState::Main:
	{
		static float time = 0.0f;
		time += TimeManager::GetInstance().GetDeltaTime();

		if (time > 1.0f) {
			time = 0.0f;
			constexpr float SPREAD = 10.0f;
			CreatePrefabRootGameObject("Enemy.json")->SetPosition(
				XMVectorSet(RNG::GetInstance().Range(-SPREAD, SPREAD), 0.0f, RNG::GetInstance().Range(-SPREAD, SPREAD), 1.0f)
			);
		}

		if (InputManager::GetInstance().GetKeyDown(KeyCode::Num0)) {
			ChangeState(EHyojeState::Result);
		}
		break;
	}

	case EHyojeState::Result:
		if (InputManager::GetInstance().GetKeyDown(KeyCode::R)) {
			ChangeState(EHyojeState::Title);
		}
		break;
	}
}

void HyojeTestScene::OnHyojeStateExit(EHyojeState type)
{
	switch (type) {
	case EHyojeState::Title:
		// [Title 정리]
		// 예: 타이틀 UI 숨기기
		break;

	case EHyojeState::Main:
		// [Main 정리]
		SoundManager::GetInstance().Stop_ChannelGroup();
		break;

	case EHyojeState::Result:
		// [Result 정리]
		// 예: 결과 UI 숨기기
		break;
	}
}




void  HyojeTestScene::BindUIActions()
{
    Panel* optionPanel = nullptr;

    for (const auto& uiPtr : m_UIList) {
        if (auto* panel = dynamic_cast<Panel*>(uiPtr.get())) {
            if (panel->GetName() == "Option") {
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
