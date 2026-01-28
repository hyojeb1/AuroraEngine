#pragma once
#include "GameObjectBase.h"

class Player : public GameObjectBase
{
	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_lineVertexBufferAndShader = {};
	com_ptr<ID3D11PixelShader> m_linePixelShader = nullptr;
	std::deque<std::pair<LineBuffer, float>> m_lineBuffers = {};

	std::vector<std::pair<float, class Enemy*>> m_deadEyeTargets = {};

	float m_moveSpeed = 5.0f;
	float m_xSensitivity = 0.1f;

	class CamRotObject* m_cameraObject = nullptr;
	float m_cameraYSensitivity = 0.1f;
	GameObjectBase* m_gunObject = nullptr;
	class FSMComponentGun* m_gunFSM = nullptr;

	bool m_isDeadEyeActive = false;
	const float m_deadEyeDuration = 0.25f;
	float m_deadEyeTime = 0.0f;

	PostProcessingBuffer m_postProcessingBuffer = {};

	size_t m_currentNodeIndex = -1;

public:
	Player() = default;
	~Player() = default;
	Player(const Player&) = default;
	Player& operator=(const Player&) = default;
	Player(Player&&) = default;
	Player& operator=(Player&&) = default;

private:
	void Initialize() override;
	void Update() override;
	void Render() override;
	void Finalize() override;

	void PlayerMove(float deltaTime, class InputManager& input);
	void PlayerShoot();
	void PlayerDeadEyeStart();
	void PlayerDeadEye(float deltaTime);
	void PlayerDeadEyeEnd();

	void RenderLineBuffers(class Renderer& renderer);
	void RenderDeadEyeTargetsUI(class Renderer& renderer);
	void RenderUINode(class Renderer& renderer);
};