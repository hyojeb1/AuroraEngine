
#include "stdafx.h"

#include "ResourceManager.h"
#include "Renderer.h"

#include "UIBase.h"


void Button::UpdateRect()
{
	Renderer& renderer = Renderer::GetInstance();
	const DXGI_SWAP_CHAIN_DESC1& swapChainDesc = renderer.GetSwapChainDesc();

	const auto worldPos = GetWorldPosition();

	const DirectX::XMFLOAT2 windowPos = { static_cast<float>(swapChainDesc.Width) * worldPos.x, static_cast<float>(swapChainDesc.Height) * worldPos.y };

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

void Panel::OnResize()
{
	UpdateRect();

	for (auto& child : m_children)
		child->OnResize();
}

void Panel::RenderUI(Renderer& renderer)
{
	if (!IsActuallyActive())
		return;

	auto tex = m_textureIdle.first;
	auto pos = GetWorldPosition();
	auto offset = m_textureIdle.second;
	auto scale = m_scale;
	auto color = m_color;
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

void UIBase::SetTextureAndOffset(const std::string& idle)
{
	m_textureIdle = ResourceManager::GetInstance().GetTextureAndOffset(idle);
	UpdateRect();
}

void Panel::UpdateRect()
{
	Renderer& renderer = Renderer::GetInstance();
	const auto& desc = renderer.GetSwapChainDesc();

	const auto worldPos = GetWorldPosition();

	const DirectX::XMFLOAT2 windowPos =
	{
		desc.Width * worldPos.x,
		desc.Height * worldPos.y
	};

	const DirectX::XMFLOAT2 offset =
	{
		m_textureIdle.second.x * m_scale,
		m_textureIdle.second.y * m_scale
	};

	m_UIRect =
	{
		(LONG)(windowPos.x - offset.x),
		(LONG)(windowPos.y - offset.y),
		(LONG)(windowPos.x + offset.x),
		(LONG)(windowPos.y + offset.y)
	};
}

void Slider::SetRange(float min, float max)
{
	m_min = min;
	m_max = max;
	m_value = std::clamp(m_value, m_min, m_max);
}

void Slider::SetValue(float newValue)
{
	if (m_value == newValue)
		return;

	m_value = newValue;
	NotifyValueChanged();
}
void Slider::SetHandleTexture(const std::string& tex)
{
	m_handleTex = ResourceManager::GetInstance().GetTextureAndOffset(tex);
}

void Slider::AddListener(std::function<void(float)> callback)
{
	listeners.push_back(callback);
}

void Slider::NotifyValueChanged()
{
	for (auto& cb : listeners)
		cb(m_value);
}

void Slider::RenderUI(Renderer& renderer)
{
	if (!IsActuallyActive())
		return;

	auto barTex = m_textureIdle.first;
	auto barPos = GetWorldPosition();
	auto barOff = m_textureIdle.second;
	auto scale = m_scale;
	auto color = m_color;
	auto depth = m_depth;

	renderer.UI_RENDER_FUNCTIONS().emplace_back(
		[barTex, barPos, barOff, scale, color, depth]()
		{
			Renderer::GetInstance().RenderImageUIPosition(
				barTex, barPos, barOff, scale, color, depth
			);
		});

	float t = (m_value - m_min) / (m_max - m_min);
	t = std::clamp(t, 0.0f, 1.0f);

	float handleX =
		m_UIRect.left + t * (m_UIRect.right - m_UIRect.left);
	float handleY =
		(m_UIRect.top + m_UIRect.bottom) * 0.5f;

	const auto& desc = Renderer::GetInstance().GetSwapChainDesc();
	DirectX::XMFLOAT2 handlePos =
	{
		handleX / desc.Width,
		handleY / desc.Height
	};

	auto handleTex = m_handleTex.first;
	auto handleOff = m_handleTex.second;

	renderer.UI_RENDER_FUNCTIONS().emplace_back(
		[handleTex, handlePos, handleOff, scale, color, depth]()
		{
			Renderer::GetInstance().RenderImageUIPosition(
				handleTex, handlePos, handleOff, scale, color, depth + 0.01f
			);
		});

	//float barLeft = m_UIRect.left;
	//float barRight = m_UIRect.right;
	//float barTop = m_UIRect.top;
	//float barBot = m_UIRect.bottom;

	//float barWidth = barRight - barLeft;
	//float barHeight = barBot - barTop;

	//float fillCenterX = barLeft + barWidth * t * 0.5f;
	//float fillCenterY = (barTop + barBot) * 0.5f;

	//const auto& desc = Renderer::GetInstance().GetSwapChainDesc();
	//DirectX::XMFLOAT2 fillPos =
	//{
	//	fillCenterX / desc.Width,
	//	fillCenterY / desc.Height
	//};

	//DirectX::XMFLOAT2 fillScale =
	//{
	//	m_scale * t,   
	//	m_scale        
	//};

	//auto fillTex = m_fillTexture.first;
	//auto fillOff = m_fillTexture.second;
	//auto color = m_color;
	//auto depth = m_depth + 0.001f;

	//renderer.UI_RENDER_FUNCTIONS().emplace_back(
	//	[fillTex, fillPos, fillOff, fillScale, color, depth]()
	//	{
	//		Renderer::GetInstance().RenderImageUIPosition(
	//			fillTex,
	//			fillPos,
	//			fillOff,
	//			fillScale,
	//			color,
	//			depth
	//		);
	//	}
	//);
}

void Slider::UpdateRect()
{
	Renderer& renderer = Renderer::GetInstance();
	const auto& desc = renderer.GetSwapChainDesc();

	const auto worldPos = GetWorldPosition();

	const DirectX::XMFLOAT2 windowPos =
	{
		desc.Width * worldPos.x,
		desc.Height * worldPos.y
	};

	const DirectX::XMFLOAT2 offset =
	{
		m_textureIdle.second.x * m_scale,
		m_textureIdle.second.y * m_scale
	};

	m_UIRect =
	{
		(LONG)(windowPos.x - offset.x),
		(LONG)(windowPos.y - offset.y),
		(LONG)(windowPos.x + offset.x),
		(LONG)(windowPos.y + offset.y)
	};
}

bool Slider::CheckInput(const POINT& mousePos, bool isMousePressed)
{
	if (!IsActuallyActive())
		return false;

	if (!isMousePressed)
	{
		m_dragging = false;
		return false;
	}

	if (!m_dragging)
	{
		if (!PtInRect(&m_UIRect, mousePos))
			return false;

		m_dragging = true;
	}

	float t =
		float(mousePos.x - m_UIRect.left) /
		float(m_UIRect.right - m_UIRect.left);

	t = std::clamp(t, 0.0f, 1.0f);
	m_value = m_min + t * (m_max - m_min);

	return true; // 입력 소비
}