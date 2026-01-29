#include "stdafx.h"

#include "GameObjectBase.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "TimeManager.h"
#include "SoundManager.h"
#include "CrossHairAndNode.h"
#include "InputManager.h"

REGISTER_TYPE(CrossHairAndNode)

void CrossHairAndNode::Initialize()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	m_crosshairTextureAndOffset = resourceManager.GetTextureAndOffset("Cross_Hair_Middle.png");
	m_RhythmLineTextureAndOffset = resourceManager.GetTextureAndOffset("Line.png");
	m_NodeAndOffset = resourceManager.GetTextureAndOffset("Line.png");

	m_NodeDataPtr = SoundManager::GetInstance().GetNodeDataPtr();

	m_CrossHairSize = 0.04f;
	m_NodeStartSize = 0.04f;
	m_NodeEndSize = 0.04f;

	m_NodeStart = 0.25f;
	m_NodeEnd = 0.5f;

	m_linePos = 0.45f;
	m_lineScl = 0.04f;
}

void CrossHairAndNode::Update()
{
	float delta = TimeManager::GetInstance().GetNSDeltaTime();
	InputManager& input = InputManager::GetInstance();

	ResizeMiddleCH(input, delta);

	GenerateNode();

	for_each(m_UINode.begin(), m_UINode.end(), [&](auto& time) { time -= TimeManager::GetInstance().GetNSDeltaTime(); });
	if (!m_UINode.empty() && m_UINode.front() < 0.0f) m_UINode.pop_front();
}

void CrossHairAndNode::Render()
{
	Renderer& renderer = Renderer::GetInstance();

	RenderCrossHair(renderer);

	if (!m_UINode.empty()) RenderUINode(renderer);
}

void CrossHairAndNode::RenderImGui()
{
	ImGui::DragFloat("Middle Size", &m_CrossHairSize, 0.004f, 0.004f, 0.04f);
	ImGui::DragFloat("Start Size", &m_NodeStartSize, 0.004f, 0.004f, 1);
	ImGui::DragFloat("End Size", &m_NodeEndSize, 0.004f, 0.004f, 1);
	ImGui::DragFloat("Node start", &m_NodeStart, 0.001f, 0.1f, 0.5f);
	ImGui::DragFloat("Node End", &m_NodeEnd, 0.001f, 0.1f, 0.5f);
	ImGui::DragFloat("Interval", &m_linePos, 0.001f, 0.1f, 0.5f);
	ImGui::DragFloat("Line Scl", &m_lineScl, 0.001f, 0.004f, 0.5f);
}

void CrossHairAndNode::Finalize()
{

}

nlohmann::json CrossHairAndNode::Serialize()
{
	nlohmann::json jsonData;
	return jsonData;
}

void CrossHairAndNode::Deserialize(const nlohmann::json& jsonData)
{

}

void CrossHairAndNode::RenderUINode(Renderer& renderer)
{
	renderer.UI_RENDER_FUNCTIONS().emplace_back
	(
		[&]()
		{
			for (const auto& time : m_UINode)
			{
				float temp = time / SoundManager::GetInstance().GetRhythmOffset();

				float pos = std::clamp(std::lerp(m_NodeEnd, m_NodeStart, temp), 0.0f, m_NodeEnd);

				float scale = std::clamp(std::lerp(m_NodeStartSize, m_NodeEndSize, temp), 0.0f, 1.0f);

				Renderer::GetInstance().RenderImageUIPosition(m_NodeAndOffset.first, { pos, 0.5f }, m_NodeAndOffset.second, scale);
				Renderer::GetInstance().RenderImageUIPosition(m_NodeAndOffset.first, { 1.0f - pos, 0.5f }, m_NodeAndOffset.second, -scale);
			}
		}
	);
}

void CrossHairAndNode::ResizeMiddleCH(InputManager& input, float delta)
{
	if (input.GetKeyDown(KeyCode::LeftBracket))
	{
		m_CrossHairSize -= 1.0f * delta;

		if (m_CrossHairSize < 0.004f)
		{
			m_CrossHairSize = 0.004f;
		}
	}

	if (input.GetKeyDown(KeyCode::RightBracket))
	{
		m_CrossHairSize += 1.0f * delta;

		if (m_CrossHairSize > 0.04f)
		{
			m_CrossHairSize = 0.04f;
		}
	}
}

void CrossHairAndNode::RenderCrossHair(Renderer& renderer)
{
	renderer.UI_RENDER_FUNCTIONS().emplace_back([&]() { Renderer::GetInstance().RenderImageUIPosition(m_crosshairTextureAndOffset.first, { 0.5f, 0.5f }, m_crosshairTextureAndOffset.second, m_CrossHairSize); });
	renderer.UI_RENDER_FUNCTIONS().emplace_back([&]() { Renderer::GetInstance().RenderImageUIPosition(m_RhythmLineTextureAndOffset.first, { m_linePos, 0.5f }, m_RhythmLineTextureAndOffset.second, m_lineScl); });
	renderer.UI_RENDER_FUNCTIONS().emplace_back([&]() { Renderer::GetInstance().RenderImageUIPosition(m_RhythmLineTextureAndOffset.first, { 1-m_linePos, 0.5f }, m_RhythmLineTextureAndOffset.second, m_lineScl); });
}

void CrossHairAndNode::GenerateNode()
{
	auto& sm = SoundManager::GetInstance();

	if (!sm.ConsumeNodeChanged())
		return;

	m_UINode.push_back(sm.GetRhythmOffset());
}
