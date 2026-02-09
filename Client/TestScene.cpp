#include "stdafx.h"
#include "TestScene.h"

#include "InputManager.h"
#include "SceneManager.h"
#include "TimeManager.h"
#include "SoundManager.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "CameraComponent.h"

#include "TestCameraObject.h"
#include "CamRotObject.h"

#include "Player.h"
#include "Enemy.h"
#include "RNG.h"

#include "Shared/Config/Option.h"

REGISTER_TYPE(TestScene)

using namespace std;
using namespace DirectX;

void TestScene::Initialize()
{
	ShowCursor(FALSE);

	m_player = dynamic_cast<Player*>(GetRootGameObject("Player"));

	SoundManager::GetInstance().Main_BGM_Shot(Config::Main_BGM,1.0f);
	SoundManager::GetInstance().Ambience_Shot(Config::Ambience);
}

void TestScene::Update()
{
	float deltaTime = TimeManager::GetInstance().GetDeltaTime();
	SpawnEnemy(deltaTime);

	if (InputManager::GetInstance().GetKeyDown(KeyCode::Num0))
	{
		SceneManager::GetInstance().ChangeScene("EndingScene");
	}

	if (SoundManager::GetInstance().CheckBGMEnd())
	{
		SoundManager::GetInstance().Main_BGM_Shot(SoundManager::GetInstance().GetCurrentTrackName(), 3.0f);
	}
}

void TestScene::Render()
{
	RenderSpawnPoints();
}

#ifdef _DEBUG
void TestScene::RenderImGui()
{
	if (ImGui::TreeNode("Spawn Points"))
	{
		for (XMVECTOR& point : m_spawnPoints) ImGui::DragFloat3(("Point " + to_string(&point - &m_spawnPoints[0])).c_str(), &point.m128_f32[0], 0.1f);
		if (ImGui::Button("Add Point")) m_spawnPoints.push_back(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));

		ImGui::TreePop();
	}
}
#endif

void TestScene::Finalize()
{
	SoundManager::GetInstance().Stop_ChannelGroup();
}

nlohmann::json TestScene::Serialize()
{
	nlohmann::json jsonData = {};
	nlohmann::json spawnPointsData = nlohmann::json::array();

	for (const XMVECTOR& point : m_spawnPoints) spawnPointsData.push_back({ XMVectorGetX(point), XMVectorGetY(point), XMVectorGetZ(point) });
	jsonData["spawnPoints"] = spawnPointsData;

	return jsonData;
}

void TestScene::Deserialize(const nlohmann::json& jsonData)
{
	m_spawnPoints.clear();
	if (jsonData.find("spawnPoints") != jsonData.end())
	{
		const nlohmann::json& spawnPointsData = jsonData["spawnPoints"];
		for (const auto& pointData : spawnPointsData)
		{
			XMVECTOR point = XMVectorSet
			(
				pointData[0].get<float>(),
				pointData[1].get<float>(),
				pointData[2].get<float>(),
				1.0f
			);
			m_spawnPoints.emplace_back(point);
		}
	}
}

constexpr float SPAWN_ACTIVE_DISTANCE_SQ = 100.0f;

void TestScene::SpawnEnemy(float deltaTime)
{
	static float spawnTime = 0.0f;
	spawnTime += deltaTime;

	if (spawnTime > m_spawnInterval)
	{
		vector<XMVECTOR> validSpawnPoints = {};
		copy_if
		(
			m_spawnPoints.begin(),
			m_spawnPoints.end(),
			back_inserter(validSpawnPoints),
			[&](const XMVECTOR& point) { return XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(GetRootGameObject("Player")->GetWorldPosition(), point))) < SPAWN_ACTIVE_DISTANCE_SQ; }
		);

		if (!validSpawnPoints.empty())
		{
			XMVECTOR spawnPoint = validSpawnPoints[RNG::GetInstance().Range(0, static_cast<int>(validSpawnPoints.size()) - 1)];
			CreatePrefabRootGameObject("Enemy.json")->SetPosition(spawnPoint);

			spawnTime = 0.0f;
		}
	}
}

void TestScene::RenderSpawnPoints()
{
	Renderer::GetInstance().UI_RENDER_FUNCTIONS().emplace_back
	(
		[&]()
		{
			pair<com_ptr<ID3D11ShaderResourceView>, XMFLOAT2> spawnPointTextureAndOffset = {};
			spawnPointTextureAndOffset = ResourceManager::GetInstance().GetTextureAndOffset("Crosshair.png");
			for (const XMVECTOR& point : m_spawnPoints)
			{
				Renderer::GetInstance().RenderImageScreenPosition
				(
					spawnPointTextureAndOffset.first,
					CameraComponent::GetMainCamera().WorldToScreenPosition(point),
					spawnPointTextureAndOffset.second,
					0.5f,
					XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(GetRootGameObject("Player")->GetWorldPosition(), point))) > SPAWN_ACTIVE_DISTANCE_SQ ? XMVECTOR{ 1.0f, 0.0f, 0.0f, 1.0f } : XMVECTOR{ 0.0f, 1.0f, 0.0f, 1.0f }
				);
			}
		}

	);
}