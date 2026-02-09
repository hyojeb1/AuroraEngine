#include "stdafx.h"
#include "UIBase.h"
#include "Panel.h"

#include "ResourceManager.h"
#include "Renderer.h"

Panel::Panel()
{
	SetTextureAndOffset("UI_Panel.png");
	SetScale(0.1f);
}

void Panel::OnResize(std::pair<float, float> res)
{
	m_resolutionScale = res.first;

	UpdateRect();

	for (auto& child : m_children)
		child->OnResize(res);
}

void Panel::RenderUI(Renderer& renderer)
{
	if (!IsActuallyActive())
		return;

	auto tex = m_textureIdle.first;
	auto pos = GetWorldPosition();
	auto offset = m_textureIdle.second;
	auto scale = GetFinalScale();
	auto color = m_colorIdle;
	auto depth = m_depth;

	renderer.UI_RENDER_FUNCTIONS().emplace_back(
		[tex, pos, offset, scale, color, depth]()
		{
			Renderer::GetInstance().RenderImageUIPosition(
				tex, pos, offset, scale, color, depth
			);
		});

	for (auto& child : m_children)
	{
		if (child->GetActive())
			child->RenderUI(renderer);
	}
}

void Panel::UpdateRect()
{
	auto pos = GetWorldPosition();

	float halfW = m_textureIdle.second.x * 0.5f * GetFinalScale();
	float halfH = m_textureIdle.second.y * 0.5f * GetFinalScale();

	m_UIRect =
	{
		static_cast<LONG>(pos.x - halfW),
		static_cast<LONG>(pos.y - halfH),
		static_cast<LONG>(pos.x + halfW),
		static_cast<LONG>(pos.y + halfH)
	};
}

void Panel::Update()
{
	UIBase::Update();
}
