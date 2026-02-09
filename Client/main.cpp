#include "stdafx.h"

#include "WindowManager.h"
#include "SceneManager.h"
#include "NavigationManager.h"
#include "RNG.h"
#include "SoundManager.h"

#include "TestScene.h"
#include "HyojeTestScene.h"
#include "TaehyeonTestScene.h"
#include "GameManager.h"

using namespace std;

int main()
{
	#ifdef _DEBUG
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
	#endif

	// 윈도우 매니저 초기화 // 렌더러, 인풋 매니저도 내부에서 초기화됨
	WindowManager& windowManager = WindowManager::GetInstance();
	windowManager.Initialize(L"Aurora");

	NavigationManager::GetInstance().Initialize();

	SceneManager& sceneManager = SceneManager::GetInstance();
	sceneManager.Initialize();
	sceneManager.ChangeScene("TitleScene");

	RNG::GetInstance().Initialize();

	SoundManager& soundManager = SoundManager::GetInstance();
	soundManager.Initialize();

	GameManager& gameManager = GameManager::GetInstance();
	gameManager.Initialize();

	while (windowManager.ProcessMessages())
	{
		gameManager.Update();
		soundManager.Update();
		sceneManager.Run();
	}

	windowManager.Finalize();

	sceneManager.Finalize();

	#ifdef _DEBUG
	ImGui::DestroyContext();
	#endif
}
