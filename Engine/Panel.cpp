#include "stdafx.h"
#include "UIBase.h"
#include "Panel.h"

#include "ResourceManager.h"
#include "Renderer.h"

Panel::Panel()
{
	SetTextureAndOffset("UI_Panel.png");
	m_scale = 0.1f;
}

void Panel::OnResize(std::pair<float, float> res)
{
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
	auto scale = m_scale;
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
	const DirectX::XMFLOAT2 windowPos = Renderer::GetInstance().ToScreenPosition(GetWorldPosition());

	const float scale = m_scale;
	const DirectX::XMFLOAT2 offset =
	{
		m_textureIdle.second.x * scale,
		m_textureIdle.second.y * scale
	};

	m_UIRect =
	{
		(LONG)(windowPos.x - offset.x),
		(LONG)(windowPos.y - offset.y),
		(LONG)(windowPos.x + offset.x),
		(LONG)(windowPos.y + offset.y)
	};
}

void Panel::Update()
{
	UIBase::Update();
}
