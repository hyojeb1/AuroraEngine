#pragma once
#include "GameObjectBase.h"

class Player : public GameObjectBase
{
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_lineVertexBufferAndShader = {};
	com_ptr<ID3D11PixelShader> m_linePixelShader = nullptr;
	std::deque<std::pair<LineBuffer, float>> m_lineBuffers = {};

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_deadEyeTextureAndOffset = {};
	std::vector<std::pair<float, class Enemy*>> m_deadEyeTargets = {};
	DirectX::XMFLOAT2 m_prevDeadEyePos = {};
	DirectX::XMFLOAT2 m_nextDeadEyePos = {};
	float m_deadEyeMoveTimer = 0.0f;
	const float m_deadEyeMoveSpeed = 200.0f;

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_enemyHitTextureAndOffset = {};
	const float m_enemyHitDisplayTime = 0.2f;
	float m_enemyHitTimer = 0.0f;

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_bulletImgs = {};

	DirectX::XMVECTOR m_inputDirection = {};
	DirectX::XMVECTOR m_normalizedMoveDirection = {};
	float m_moveSpeed = 5.0f;

	class CameraComponent* m_cameraComponent = nullptr;
	float m_cameraSensitivity = 0.1f;
	GameObjectBase* m_gunObject = nullptr;
	class FSMComponentGun* m_gunFSM = nullptr;

	bool m_isDeadEyeActive = false;
	float m_deadEyeTotalDuration = 0.0f;
	float m_deadEyeDuration = 0.0f;

	PostProcessingBuffer m_postProcessingBuffer = {};

	size_t m_currentNodeIndex = -1;
	size_t m_DeadEyeCount = 0;

	int m_MaxBullet = 6;
	int m_bulletCnt = 0;

	std::pair<float, float> m_bulletUIpos{ 0.0f,0.0f };
	float m_bulletInterval = 0.0f;

	bool m_isDashing = false;
	float m_dashTimer = 0.0f;
	DirectX::XMVECTOR m_dashDirection = {};
	const float m_kDashSpeed = 30.0f;
	const float m_kDashDuration = 0.15f; 

	bool m_lutCrossfadeActive = false;
	bool m_lutCrossfadeReverse = false;
	float m_lutCrossfadeElapsed = 0.0f;
	float m_lutCrossfadeDuration = 0.18f;

public:
	Player() = default;
	~Player() = default;
	Player(const Player&) = default;
	Player& operator=(const Player&) = default;
	Player(Player&&) = default;
	Player& operator=(Player&&) = default;

	bool GetActiveDeadEye() const { return m_isDeadEyeActive; }

private:
	void Initialize() override;
	void Update() override;
	void Render() override;
	void Finalize() override;

	void UpdateMoveDirection(class InputManager& input);
	void PlayerTriggerDash(class InputManager& input);
	void PlayerDash(float deltaTime, class InputManager& input);
	void PlayerShoot();
	void PlayerReload(int cnt);
	void PlayerDeadEyeStart();
	void PlayerDeadEye(float deltaTime, class InputManager& input);
	void PlayerDeadEyeEnd();

	void RenderLineBuffers(class Renderer& renderer);
	void RenderDeadEyeTargetsUI(class Renderer& renderer);
	void RenderEnemyHitUI(class Renderer& renderer);
	void RenderUINode(class Renderer& renderer);
	void RenderBullets(class Renderer& renderer);


	void UpdateLutCrossfade(float deltaTime);
	void TriggerLUT();
};