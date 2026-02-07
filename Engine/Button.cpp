#include "stdafx.h"
#include "UIBase.h"
#include "Button.h"

#include "ResourceManager.h"
#include "Renderer.h"

Button::Button()
{
	SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Cicked.png");
	m_scale = 1.0f;
}

void Button::UpdateRect()
{
	const DirectX::XMFLOAT2 windowPos = Renderer::GetInstance().ToScreenPosition(GetWorldPosition());
	const DirectX::XMFLOAT2 offset = { m_textureIdle.second.x * m_scale, m_textureIdle.second.y * m_scale };

	m_UIRect =
	{
		static_cast<LONG>(windowPos.x - offset.x),
		static_cast<LONG>(windowPos.y - offset.y),
		static_cast<LONG>(windowPos.x + offset.x),
		static_cast<LONG>(windowPos.y + offset.y)
	};
}
void Button::SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed)
{
	m_textureIdle = m_textureClicked = ResourceManager::GetInstance().GetTextureAndOffset(idle);
	m_textureHoverd = ResourceManager::GetInstance().GetTextureAndOffset(hoverd);
	m_texturePressed = ResourceManager::GetInstance().GetTextureAndOffset(pressed);
	UpdateRect();
}

void Button::SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed, const std::string& clicked)
{
	m_textureIdle = ResourceManager::GetInstance().GetTextureAndOffset(idle);
	m_textureHoverd = ResourceManager::GetInstance().GetTextureAndOffset(hoverd);
	m_texturePressed = ResourceManager::GetInstance().GetTextureAndOffset(pressed);
	m_textureClicked = ResourceManager::GetInstance().GetTextureAndOffset(clicked);
	UpdateRect();
}
void Button::RenderUI(class Renderer& renderer)
{
	if (!IsActuallyActive())
		return;

	auto texidle = m_textureIdle.first;
	auto texhoverd = m_textureHoverd.first;
	auto texpressed = m_texturePressed.first;
	auto texclicked = m_textureClicked.first;
	auto pos = GetWorldPosition();
	auto offset = m_textureIdle.second;
	auto scale = m_scale;
	auto color = m_color;
	auto depth = m_depth;

	switch (m_ButtonState)
	{
	case Button::ButtonState::Idle:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texidle, pos, offset, scale, color, depth]() { Renderer::GetInstance().RenderImageUIPosition(texidle, pos, offset, scale, color, depth); });
		break;
	case Button::ButtonState::Hoverd:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texhoverd, pos, offset, scale, color, depth]() { Renderer::GetInstance().RenderImageUIPosition(texhoverd, pos, offset, scale, color, depth); });
		break;
	case Button::ButtonState::Pressed:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texpressed, pos, offset, scale, color, depth]() { Renderer::GetInstance().RenderImageUIPosition(texpressed, pos, offset, scale, color, depth); });
		break;
	case Button::ButtonState::Clicked:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texclicked, pos, offset, scale, color, depth]() { Renderer::GetInstance().RenderImageUIPosition(texclicked, pos, offset, scale, color, depth); });
		break;
	}
}

bool Button::CheckInput(const POINT& mousePosition, bool isMousePressed, bool isMouseClicked)
{
	if (!IsActuallyActive())
		return false;

	if (m_UIRect.left <= mousePosition.x && mousePosition.x <= m_UIRect.right && m_UIRect.top <= mousePosition.y && mousePosition.y <= m_UIRect.bottom)
	{
		m_ButtonState = ButtonState::Hoverd;
		if (isMousePressed) m_ButtonState = ButtonState::Pressed;
		else if (isMouseClicked && m_onClick)
		{
			m_ButtonState = ButtonState::Clicked;
			m_onClick();
		}
	}
	else
	{
		m_ButtonState = ButtonState::Idle;
		return false;
	}

	return true;
}
