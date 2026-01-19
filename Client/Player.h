#pragma once
#include "GameObjectBase.h"

class Player : public GameObjectBase
{
	LineBuffer m_lineBufferData = {};
	float m_lineDisplayTime = 0.0f;

	std::pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> m_lineVertexBufferAndShader = {};
	com_ptr<ID3D11PixelShader> m_linePixelShader = nullptr;

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