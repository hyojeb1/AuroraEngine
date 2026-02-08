/// HyojeTestScene.h의 시작
#pragma once
#include "SceneBase.h"

enum EHyojeState
{
	Title,
	Main,
	Result,
};

namespace
{
	constexpr int kPlayerHP = 3;
	constexpr float kHitCooldownSec = 1.0f;
}

class HyojeTestScene : public SceneBase
{
private:
	int m_player_hp = kPlayerHP;
	float m_hitCooldownTimer = 0.0f;
	
	EHyojeState m_currentState = EHyojeState::Title;

	class Panel* optionPanel = nullptr;
	class Panel* titlePanel = nullptr;
	class Panel* resultPanel = nullptr;


public:
	void ChangeState(EHyojeState newState);

	void OnHyojeStateEnter(EHyojeState type);
	void OnHyojeStateUpdate();
	void OnHyojeStateExit(EHyojeState type);

	void OnPlayerHit(int damage);

protected:
	virtual void Initialize() override;
	virtual void Update() override;
	virtual void Finalize() override;

	void BindUIActions() override;

public:
	HyojeTestScene() = default;
	~HyojeTestScene() override = default;
	HyojeTestScene(const HyojeTestScene&) = default;
	HyojeTestScene& operator=(const HyojeTestScene&) = default;
	HyojeTestScene(HyojeTestScene&&) = default;
	HyojeTestScene& operator=(HyojeTestScene&&) = default;
};
/// HyojeTestScene.h의 끝
