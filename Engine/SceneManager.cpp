#include "stdafx.h"
#include "SceneManager.h"

#include "Renderer.h"

using namespace std;

void SceneManager::Run()
{
	if (m_nextScene)
	{
		if (m_currentScene) m_currentScene->BaseFinalize();
		m_currentScene = move(m_nextScene);
		m_currentScene->BaseInitialize();
	}

	m_currentScene->BaseUpdate(GetDeltaTime());

	m_renderer->BeginFrame();

	// ¾À ·»´õ¸µ
	m_currentScene->BaseRender();
	// ImGui ·»´õ¸µ
	m_currentScene->BaseRenderImGui();

	m_renderer->EndFrame();
}

float SceneManager::GetDeltaTime()
{
	static DWORD previousTime = timeGetTime();
	const DWORD currentTime = timeGetTime();
	const float deltaTime = static_cast<float>(currentTime - previousTime) / 1000.0f;
	previousTime = currentTime;

	return deltaTime;
}