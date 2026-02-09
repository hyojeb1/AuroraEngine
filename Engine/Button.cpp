#include "stdafx.h"
#include "UIBase.h"
#include "Button.h"

#include "ResourceManager.h"
#include "Renderer.h"

using namespace DirectX;

Button::Button()
{
	SetTextureAndOffset("UI_IDLE.png", "UI_Hovered.png", "UI_Pressed.png", "UI_Clicked.png");
}

void Button::UpdateRect()
{
	const DirectX::XMFLOAT2 windowPos = Renderer::GetInstance().ToScreenPosition(GetWorldPosition());
	const DirectX::XMFLOAT2 offset = { m_textureIdle.second.x * m_scale,
		m_textureIdle.second.y * m_scale };

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
	auto offset_idle	= m_textureIdle.second;
	auto offset_hoverd	= m_textureHoverd.second;
	auto offset_pressed = m_texturePressed.second;
	auto offset_clicked = m_textureClicked.second;
	auto scale_idle	   = m_scale;
	auto scale_hoverd = m_scale * m_scaleHover  ;
	auto scale_pressed = m_scale *m_scalePressed;
	auto scale_clicked = m_scale *m_scaleClicked;
	auto color_idle	   = m_colorIdle;
	auto color_hoverd  =  m_colorHover ;
	auto color_pressed =  m_colorPressed;
	auto color_clicked =  m_colorClicked;
	auto depth = m_depth;
	RECT currentAnimRect = m_UIAnimationRect;
	switch (m_ButtonState)
	{
	case Button::ButtonState::Idle:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texidle, pos, offset_idle, scale_idle, color_idle, depth, currentAnimRect]() {
			const RECT* pSrcRect =  &currentAnimRect;
			//Renderer::GetInstance().RenderImageUIPosition(texidle, pos, offset_idle, scale_idle, color_idle, depth, pSrcRect);
			Renderer::GetInstance().RenderImageUIPosition(texidle, pos, offset_idle, scale_idle, color_idle, depth, nullptr);
			});
		break;
	case Button::ButtonState::Hoverd:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texhoverd, pos, offset_hoverd, scale_hoverd, color_hoverd, depth, currentAnimRect]() { 
			const RECT* pSrcRect = &currentAnimRect;
			//Renderer::GetInstance().RenderImageUIPosition(texhoverd, pos, offset_hoverd, scale_hoverd, color_hoverd, depth, pSrcRect);
			Renderer::GetInstance().RenderImageUIPosition(texhoverd, pos, offset_hoverd, scale_hoverd, color_hoverd, depth, nullptr);
			});
		break;
	case Button::ButtonState::Pressed:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texpressed, pos, offset_pressed, scale_pressed, color_pressed, depth, currentAnimRect]() {
			const RECT* pSrcRect = &currentAnimRect;
			//Renderer::GetInstance().RenderImageUIPosition(texpressed, pos, offset_pressed, scale_pressed, color_pressed, depth, pSrcRect);
			Renderer::GetInstance().RenderImageUIPosition(texpressed, pos, offset_pressed, scale_pressed, color_pressed, depth, nullptr);
			});
		break;
	case Button::ButtonState::Clicked:
		renderer.UI_RENDER_FUNCTIONS().emplace_back([texclicked, pos, offset_clicked, scale_clicked, color_clicked, depth, currentAnimRect]() { 
			const RECT* pSrcRect = &currentAnimRect;
			//Renderer::GetInstance().RenderImageUIPosition(texclicked, pos, offset_clicked, scale_clicked, color_clicked, depth, pSrcRect);
			Renderer::GetInstance().RenderImageUIPosition(texclicked, pos, offset_clicked, scale_clicked, color_clicked, depth, nullptr);
			});
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

	data["pathHover"] = m_pathHover;
	data["pathPressed"] = m_pathPressed;
	data["pathClicked"] = m_pathClicked;

	data["scaleHover"] = m_scaleHover;
	data["scalePressed"] = m_scalePressed;
	data["scaleClicked"] = m_scaleClicked;

	XMFLOAT4 c_h; XMStoreFloat4(&c_h , m_colorHover);
	XMFLOAT4 c_p; XMStoreFloat4(&c_p , m_colorPressed);
	XMFLOAT4 c_c; XMStoreFloat4(&c_c , m_colorClicked);
	data["colorHover"] = { c_h.x, c_h.y, c_h.z, c_h.w };
	data["colorPressed"] = { c_p.x, c_p.y, c_p.z, c_p.w };
	data["colorClicked"] = { c_c.x, c_c.y, c_c.z, c_c.w };

	if (!m_onClickActionKey.empty()) data["actionKey"] = m_onClickActionKey;

	return data;
}

void Button::Deserialize(const nlohmann::json& jsonData)
{
	UIBase::Deserialize(jsonData);

	std::string idle = jsonData.value("pathIdle", "");
	std::string hover = jsonData.value("pathHover", "");  
	std::string pressed = jsonData.value("pathPressed", ""); 
	std::string clicked = jsonData.value("pathClicked", ""); 

	if (!idle.empty()) {
		if (clicked.empty() || clicked == idle) SetTextureAndOffset(idle, hover, pressed);
		else									SetTextureAndOffset(idle, hover, pressed, clicked);
	}

	if (jsonData.contains("actionKey")) {
		m_onClickActionKey = jsonData["actionKey"];
	}

	auto ReadColor = [&](const char* key, DirectX::XMFLOAT4& target) {
		if (jsonData.contains(key)) {
			target.x = jsonData[key][0]; target.y = jsonData[key][1];
			target.z = jsonData[key][2]; target.w = jsonData[key][3];
		}
		};

	if (jsonData.contains("scaleHover")) m_scaleHover = jsonData["scaleHover"];
	if (jsonData.contains("scalePressed")) m_scalePressed = jsonData["scalePressed"];
	if (jsonData.contains("scaleClicked")) m_scaleClicked = jsonData["scaleClicked"];


	XMFLOAT4 c_h;
	XMFLOAT4 c_p;
	XMFLOAT4 c_c;
	ReadColor("colorHover", c_h);
	ReadColor("colorPressed", c_p);
	ReadColor("colorClicked", c_c);
	m_colorHover = 	XMLoadFloat4(&c_h);
	m_colorPressed = 	XMLoadFloat4(&c_p);
	m_colorClicked = 	XMLoadFloat4(&c_c);

}