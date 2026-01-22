#pragma once
#include "Singleton.h"
#include "SceneBase.h"

class SceneManager : public Singleton<SceneManager>
{
	friend class Singleton<SceneManager>;

	std::unique_ptr<class Base> m_currentScene = nullptr;
	std::unique_ptr<class Base> m_nextScene = nullptr;

	double m_accumulator = 0.0;

public:
	~SceneManager() = default;
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	SceneManager(SceneManager&&) = delete;
	SceneManager& operator=(SceneManager&&) = delete;

	void Initialize();
	void Run();
	void Finalize() { if (m_currentScene) m_currentScene->BaseFinalize(); }

	void ChangeScene(const std::string& sceneTypeName) { m_nextScene = TypeRegistry::GetInstance().CreateScene(sceneTypeName); m_accumulator = 0; }

	SceneBase* GetCurrentScene() { return dynamic_cast<SceneBase*>(m_currentScene.get()); }

private:
	SceneManager() = default;
};
