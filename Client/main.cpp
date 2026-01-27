#include "stdafx.h"

#include "WindowManager.h"
#include "SceneManager.h"
#include <SoundManager.h>

#include "TestScene.h"
#include "HyojeTestScene.h"
#include "TaehyeonTestScene.h"

using namespace std;

int main()
{
	cout << "==================================" << endl;
	cout << "Welcome to Aurora Engine" << endl;
	cout << "==================================" << endl;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

	WindowManager& windowManager = WindowManager::GetInstance();
	windowManager.Initialize(L"Aurora");

	SceneManager& sceneManager = SceneManager::GetInstance();
	sceneManager.ChangeScene("TitleScene");

	SoundManager& soundManager = SoundManager::GetInstance();
	soundManager.Initialize();

	while (windowManager.ProcessMessages()) sceneManager.Run();

	windowManager.Finalize();

	ImGui::DestroyContext();
}