#include "stdafx.h"

#include "ResourceManager.h"
#include "Renderer.h"

#include "UIBase.h"
//
//void Button::SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed)
//{
//	m_textureIdle = m_textureClicked = ResourceManager::GetInstance().GetTextureAndOffset(idle);
//	m_textureHoverd = ResourceManager::GetInstance().GetTextureAndOffset(hoverd);
//	m_texturePressed = ResourceManager::GetInstance().GetTextureAndOffset(pressed);
//	UpdateRect();
//}
//
//void Button::SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed, const std::string& clicked)
//{
//	m_textureIdle = ResourceManager::GetInstance().GetTextureAndOffset(idle);
//	m_textureHoverd = ResourceManager::GetInstance().GetTextureAndOffset(hoverd);
//	m_texturePressed = ResourceManager::GetInstance().GetTextureAndOffset(pressed);
//	m_textureClicked = ResourceManager::GetInstance().GetTextureAndOffset(clicked);
//	UpdateRect();
//}
//
//void Button::RenderButton(Renderer& renderer)
//{
//	if (!m_isActive) return;
//
//	switch (m_ButtonState)
//	{
//	case Button::ButtonState::Idle:
//		renderer.UI_RENDER_FUNCTIONS().emplace_back([&]() { Renderer::GetInstance().RenderImageUIPosition(m_textureIdle.first, m_UIPosition, m_textureHoverd.second, m_scale, m_color, m_depth); });
//		break;
//	case Button::ButtonState::Hoverd:
//		renderer.UI_RENDER_FUNCTIONS().emplace_back([&]() { Renderer::GetInstance().RenderImageUIPosition(m_textureHoverd.first, m_UIPosition, m_textureHoverd.second, m_scale, m_color, m_depth); });
//		break;
//	case Button::ButtonState::Pressed:
//		renderer.UI_RENDER_FUNCTIONS().emplace_back([&]() { Renderer::GetInstance().RenderImageUIPosition(m_texturePressed.first, m_UIPosition, m_textureHoverd.second, m_scale, m_color, m_depth); });
//		break;
//	case Button::ButtonState::Clicked:
//		renderer.UI_RENDER_FUNCTIONS().emplace_back([&]() { Renderer::GetInstance().RenderImageUIPosition(m_textureClicked.first, m_UIPosition, m_textureHoverd.second, m_scale, m_color, m_depth); });
//		break;
//	}
//
//
//}
//
//void Button::CheckInput(const POINT& mousePosition, bool isMouseClicked, bool isMousePressed)
//{
//	if (!m_isActive) return;
//
//	if (m_buttonRect.left <= mousePosition.x && mousePosition.x <= m_buttonRect.right && m_buttonRect.top <= mousePosition.y && mousePosition.y <= m_buttonRect.bottom)
//	{
//		//m_isHoverd = true;
//		m_ButtonState = ButtonState::Hoverd;
//		if (isMousePressed) m_ButtonState = ButtonState::Pressed;
//		else if (isMouseClicked && m_onClick)
//		{
//			m_ButtonState = ButtonState::Clicked;
//			m_onClick();
//		}
//	}
//	else
//	{
//		//m_isHoverd = false;
//		m_ButtonState = ButtonState::Idle;
//	}
//}
//
//void Button::UpdateRect()
//{
//	Renderer& renderer = Renderer::GetInstance();
//	const DXGI_SWAP_CHAIN_DESC1& swapChainDesc = renderer.GetSwapChainDesc();
//
//	const XMFLOAT2 windowPos = { static_cast<float>(swapChainDesc.Width) * m_UIPosition.x, static_cast<float>(swapChainDesc.Height) * m_UIPosition.y };
//	const XMFLOAT2 offset = { m_textureIdle.second.x * m_scale, m_textureIdle.second.y * m_scale };
//
//	m_buttonRect =
//	{
//		static_cast<LONG>(windowPos.x - offset.x),
//		static_cast<LONG>(windowPos.y - offset.y),
//		static_cast<LONG>(windowPos.x + offset.x),
//		static_cast<LONG>(windowPos.y + offset.y)
//	};
//}