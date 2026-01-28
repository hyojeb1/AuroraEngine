#include "stdafx.h"
#include "SceneManager.h"
#include "InputManager.h"

#include "Renderer.h"
#include "TimeManager.h"
#include "SoundManager.h"

using namespace std;

constexpr int maxSteps = 4;
constexpr double fixedDt = 0.016; // 60fps 기준

void SceneManager::Initialize()
{
	TimeManager::GetInstance().Initialize();
}

void SceneManager::Run()
{
	InputManager& inputManager = InputManager::GetInstance();

	if (m_nextScene)
	{
		if (m_currentScene) m_currentScene->BaseFinalize();
		m_currentScene = move(m_nextScene);
		m_currentScene->BaseInitialize();
	}

	TimeManager::GetInstance().UpdateTime();
	//SoundManager::GetInstance().Update();

	m_accumulator += TimeManager::GetInstance().GetDeltaTime();

	int steps = 0;
	while (m_accumulator >= fixedDt && steps < maxSteps)
	{
		m_currentScene->BaseFixedUpdate();
		m_accumulator -= fixedDt;
		steps++;
	}

	m_currentScene->BaseUpdate();

	Renderer& m_renderer = Renderer::GetInstance();
	m_renderer.BeginFrame();

	#ifdef _DEBUG
	m_currentScene->BaseRenderImGui();
	#endif

	m_currentScene->BaseRender();

	m_renderer.EndFrame();
	inputManager.EndFrame();
}
