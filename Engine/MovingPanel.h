#pragma once

#include "Panel.h"

class MovingPanel : public Panel
{
public:
	const char* GetTypeName() const override { return "MovingPanel"; }
	nlohmann::json Serialize() const override;
	void Deserialize(const nlohmann::json& jsonData) override;

	enum class MoveState
	{
		Idle,
		Moving
	};

	void UpdateUI(float deltaTime) override;

	void StartMove(const DirectX::XMFLOAT2& from, const DirectX::XMFLOAT2& to, float duration, bool deactivateOnFinish = false);
	void MoveTo(const DirectX::XMFLOAT2& to, float duration, bool deactivateOnFinish = false);

	bool IsAnimating() const { return m_state != MoveState::Idle; }
	MoveState GetState() const { return m_state; }

	void SetUseEasing(bool useEasing) { m_useEasing = useEasing; }

private:
	static float EaseOutCubic(float t);

	DirectX::XMFLOAT2 m_startPos = {};
	DirectX::XMFLOAT2 m_targetPos = {};
	float m_elapsed = 0.0f;
	float m_duration = 0.0f;
	bool m_deactivateOnFinish = false;
	bool m_useEasing = true;
	MoveState m_state = MoveState::Idle;
};
