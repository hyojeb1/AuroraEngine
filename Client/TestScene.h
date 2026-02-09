// TestScene.h의 시작
#pragma once
#include "SceneBase.h"

class TestScene : public SceneBase
{
	class Player* m_player = nullptr;

	float m_spawnInterval = 3.0f;
	std::vector<DirectX::XMVECTOR> m_spawnPoints = {};

public:
	TestScene() = default;
	~TestScene() override = default;
	TestScene(const TestScene&) = default;
	TestScene& operator=(const TestScene&) = default;
	TestScene(TestScene&&) = default;
	TestScene& operator=(TestScene&&) = default;

private:
	void Initialize() override;
	void Update() override;
	void Render() override;
	#ifdef _DEBUG
	void RenderImGui() override;
	#endif
	void Finalize() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

	void SpawnEnemy(float deltaTime);

	void RenderScore();
	void RenderSpawnPoints();
};