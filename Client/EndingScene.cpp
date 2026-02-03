
#include "stdafx.h"
#include "EndingScene.h"

#include "SceneManager.h"
#include "TimeManager.h"
#include "SoundManager.h"

#include "CameraComponent.h"

#include "UIBase.h"

REGISTER_TYPE(EndingScene)

using namespace std;
using namespace DirectX;

void EndingScene::Initialize()
{
	GetRootGameObject("MainCam")->GetComponent<class CameraComponent>()->SetAsMainCamera();

	ShowCursor(TRUE);

	Panel* TestPanel = CreatePanel();
	TestPanel->SetTextureAndOffset("test_wood2_BaseColor.png");
	TestPanel->SetLocalPosition({ 0.5f,0.5f });
	TestPanel->SetScale(0.5f);

	Button* TitleButton = CreateButton();
	//TitleButton->SetParent(TestPanel);
	TitleButton->SetTextureAndOffset("IDLE.png", "HOVERD.png", "PRESSED.png", "CLICKED.png");
	TitleButton->SetLocalPosition({ 0.0f, 0.0f });
	TitleButton->SetOnClick([TestPanel]() { TestPanel->SetActive(true); });
	TitleButton->SetScale(0.3f);

	Button* PanelCloseButton = CreateButton();
	PanelCloseButton->SetParent(TestPanel);
	PanelCloseButton->SetTextureAndOffset("IDLE.png", "HOVERD.png", "PRESSED.png", "CLICKED.png");
	PanelCloseButton->SetLocalPosition({ 0.0f, 0.1f });
	PanelCloseButton->SetOnClick([TestPanel]() { TestPanel->SetActive(false); });
	PanelCloseButton->SetScale(0.3f);
}

void EndingScene::Update()
{
}