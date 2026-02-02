#include "stdafx.h"
#include "SceneManager.h"

#include "SceneBase.h"
#include "Renderer.h"
#include "TimeManager.h"

using namespace std;

constexpr int maxSteps = 4;
constexpr double fixedDt = 0.016; // 60fps 기준

void SceneManager::Initialize()
{
	TimeManager::GetInstance().Initialize();
	LoadAllPrefabs();
}

void SceneManager::Run()
{
	if (m_nextScene)
	{
		if (m_currentScene) m_currentScene->BaseFinalize();
		m_currentScene = move(m_nextScene);
		m_currentScene->BaseInitialize();
	}

	TimeManager::GetInstance().UpdateTime();

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
}

void SceneManager::Finalize()
{
	if (m_currentScene) m_currentScene->BaseFinalize();
}

void SceneManager::ChangeScene(const string& sceneTypeName)
{
	m_nextScene = TypeRegistry::GetInstance().CreateScene(sceneTypeName);
	m_accumulator = 0;
}

SceneBase* SceneManager::GetCurrentScene()
{
	return dynamic_cast<SceneBase*>(m_currentScene.get());
}

const nlohmann::json* SceneManager::GetPrefabData(const string& prefabName)
{
	auto it = m_prefabCache.find(prefabName);
	if (it != m_prefabCache.end()) return &it->second;

	return nullptr;
}

void SceneManager::LoadAllPrefabs()
{
	const filesystem::path prefabDirectory = "../Asset/Prefab/";
	if (!filesystem::exists(prefabDirectory)) return;

	for (const auto& entry : filesystem::directory_iterator(prefabDirectory))
	{
		if (entry.path().extension() == ".json")
		{
			ifstream file(entry.path());
			nlohmann::json prefabData;
			file >> prefabData;
			file.close();
			m_prefabCache[entry.path().filename().string()] = prefabData;
		}
	}
}
