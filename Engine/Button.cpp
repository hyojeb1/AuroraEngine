#include "stdafx.h"
#include "UIBase.h"
#include "Button.h"

#include "ResourceManager.h"
#include "Renderer.h"

Button::Button()
{
	SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Clicked.png");
	m_scale = 0.1f;
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

	auto texidle	 = m_textureIdle.first;
	auto texhoverd	 = m_textureHoverd.first;
	auto texpressed	 = m_texturePressed.first;
	auto texclicked	 = m_textureClicked.first;
	auto pos = GetWorldPosition();
	auto offset_idle = m_textureIdle.second;
	auto offset_hoverd = m_textureHoverd.second;
	auto offset_pressed = m_texturePressed.second;
	auto offset_clicked = m_textureClicked.second;
	auto scale = m_scale;
	auto color = m_color;
	auto depth = m_depth;

	switch (m_ButtonState)
	{
	case Button::ButtonState::Idle:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texidle, pos, offset_idle, scale, color, depth]() { Renderer::GetInstance().RenderImageUIPosition(texidle, pos, offset_idle, scale, color, depth); });
		break;
	case Button::ButtonState::Hoverd:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texhoverd, pos, offset_hoverd, scale, color, depth]() { Renderer::GetInstance().RenderImageUIPosition(texhoverd, pos, offset_hoverd, scale, color, depth); });
		break;
	case Button::ButtonState::Pressed:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texpressed, pos, offset_pressed, scale, color, depth]() { Renderer::GetInstance().RenderImageUIPosition(texpressed, pos, offset_pressed, scale, color, depth); });
		break;
	case Button::ButtonState::Clicked:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texclicked, pos, offset_clicked, scale, color, depth]() { Renderer::GetInstance().RenderImageUIPosition(texclicked, pos, offset_clicked, scale, color, depth); });
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

void Button::SetButtonTextures(const std::string& idle, const std::string& hover, const std::string& pressed, const std::string& clicked)
{
	m_pathIdle = idle;
	m_pathHover = hover;
	m_pathPressed = pressed;
	m_pathClicked = clicked;

	auto& rm = ResourceManager::GetInstance();
	if (!idle.empty()) 
	{
		auto res = rm.GetTextureAndOffset(idle);
		m_textureIdle = res;
	}
	if (!hover.empty()) {
		auto res = rm.GetTextureAndOffset(hover);
		m_textureHoverd = res;
	}
	if (!pressed.empty()) 
	{
		auto res = rm.GetTextureAndOffset(pressed);
		m_texturePressed = res;
	}
	if (!clicked.empty())
	{
		auto res = rm.GetTextureAndOffset(clicked);
		m_textureClicked = res;
	}
}

nlohmann::json Button::Serialize() const
{
	nlohmann::json data = UIBase::Serialize();

	data["textureHover"] = m_pathHover;
	data["texturePressed"] = m_pathPressed;
	data["textureClicked"] = m_pathClicked;

	if (!m_onClickActionKey.empty()) data["actionKey"] = m_onClickActionKey;

	return data;
}

void Button::Deserialize(const nlohmann::json& jsonData)
{
	UIBase::Deserialize(jsonData);

	std::string idle = jsonData.value("textureIdle", "");
	std::string hover = jsonData.value("textureHover", "");
	std::string pressed = jsonData.value("texturePressed", "");
	std::string clicked = jsonData.value("textureClicked", "");

	if (!idle.empty()) {
		if (clicked.empty() || clicked == idle) SetTextureAndOffset(idle, hover, pressed);
		else									SetTextureAndOffset(idle, hover, pressed, clicked);
	}

	if (jsonData.contains("actionKey")) {
		m_onClickActionKey = jsonData["actionKey"];
	}

}