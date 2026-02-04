#pragma once

class SceneManager : public Singleton<SceneManager>
{
	friend class Singleton<SceneManager>;

	std::unique_ptr<class Base> m_currentScene = nullptr;
	std::unique_ptr<class Base> m_nextScene = nullptr;

	double m_accumulator = 0.0;

	std::unordered_map<std::string, nlohmann::json> m_prefabCache = {}; // 프리팹 캐시 맵

public:
	~SceneManager() = default;
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	SceneManager(SceneManager&&) = delete;
	SceneManager& operator=(SceneManager&&) = delete;

	void Initialize();
	void Run();
	void Finalize();

	void ChangeScene(const std::string& sceneTypeName);

	class SceneBase* GetCurrentScene();

	void LoadAllPrefabs();
	const nlohmann::json* GetPrefabData(const std::string& prefabName);

private:
	SceneManager() = default;
};
