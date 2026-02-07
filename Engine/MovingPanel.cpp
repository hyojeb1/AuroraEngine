#include "stdafx.h"
#include "MovingPanel.h"

using namespace DirectX;
using json = nlohmann::json;

json MovingPanel::Serialize() const
{
	json jsonData = SerializeBase();
	jsonData["useEasing"] = m_useEasing;
	return jsonData;
}

void MovingPanel::Deserialize(const json& jsonData)
{
	DeserializeBase(jsonData, true);
	if (jsonData.contains("useEasing")) m_useEasing = jsonData["useEasing"].get<bool>();
}

void MovingPanel::StartMove(const XMFLOAT2& from, const XMFLOAT2& to, float duration, bool deactivateOnFinish)
{
	m_startPos = from;
	m_targetPos = to;
	m_elapsed = 0.0f;
	m_duration = std::max(duration, 0.0001f);
	m_deactivateOnFinish = deactivateOnFinish;
	m_state = MoveState::Moving;

	SetActive(true);
	SetLocalPosition(from);
	UpdateChildrenRect();
}

void MovingPanel::MoveTo(const XMFLOAT2& to, float duration, bool deactivateOnFinish)
{
	StartMove(m_localPosition, to, duration, deactivateOnFinish);
}

void MovingPanel::UpdateUI(float deltaTime)
{
	if (m_state == MoveState::Idle)
		return;

	m_elapsed += deltaTime;
	float t = std::clamp(m_elapsed / m_duration, 0.0f, 1.0f);

	if (m_useEasing)
		t = EaseOutCubic(t);

	const XMFLOAT2 pos =
	{
		m_startPos.x + (m_targetPos.x - m_startPos.x) * t,
		m_startPos.y + (m_targetPos.y - m_startPos.y) * t
	};

	SetLocalPosition(pos);
	UpdateChildrenRect();

	if (m_elapsed >= m_duration)
	{
		m_state = MoveState::Idle;
		if (m_deactivateOnFinish)
			SetActive(false);
	}
}

float MovingPanel::EaseOutCubic(float t)
{
	const float inv = 1.0f - t;
	return 1.0f - inv * inv * inv;
}
