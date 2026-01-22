#pragma once
#include "GameObjectBase.h"

class Player : public GameObjectBase
{
	std::deque<std::pair<LineBuffer, float>> m_lineBuffers = {}; // 선 버퍼 및 남은 표시 시간 큐

	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_lineVertexBufferAndShader = {};
	com_ptr<ID3D11PixelShader> m_linePixelShader = nullptr;

	com_ptr<ID3D11ShaderResourceView> m_crosshairSRV = nullptr;
	DirectX::XMFLOAT2 m_crosshairOffset = {};

	float m_moveSpeed = 5.0f;
	float m_xSensitivity = 0.1f;

	class CamRotObject* m_cameraObject = nullptr;
	float m_cameraYSensitivity = 0.1f;
	GameObjectBase* m_gunObject = nullptr;

	bool m_isDeadEyeActive = false;
	const float m_deadEyeDuration = 0.25f;
	float m_deadEyeTime = 0.0f;
	std::vector<std::pair<float, class Enemy*>> m_deadEyeTargets = {};

	PostProcessingBuffer m_postProcessingBuffer = {};

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
};