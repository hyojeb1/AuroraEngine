#pragma once
#include "GameObjectBase.h"

class Player : public GameObjectBase
{
	std::deque<std::pair<LineBuffer, float>> m_lineBuffers = {}; // 선 버퍼 및 남은 표시 시간 큐

	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_lineVertexBufferAndShader = {};
	com_ptr<ID3D11PixelShader> m_linePixelShader = nullptr;

	GameObjectBase* m_cameraObject = nullptr;
	GameObjectBase* m_gunObject = nullptr;

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
};