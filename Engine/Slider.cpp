#include "stdafx.h"
#include "UIBase.h"
#include "Slider.h"

#include "ResourceManager.h"
#include "Renderer.h"

Slider::Slider()
{
	SetTextureAndOffset("UI_Slider_box.png"); 
	SetHandleTextures("UI_Slider.png", "UI_Slider.png", "UI_Slider.png");
}

void Slider::SetRange(float min, float max)
{
	m_min = min;
	m_max = max;
	m_value = std::clamp(m_value, m_min, m_max);
	UpdateHandleRect();
}

void Slider::SetValue(float newValue)
{
	if (m_value == newValue)
		return;

	m_value = newValue;
	UpdateHandleRect();
	NotifyValueChanged();
}
void Slider::SetHandleTexture(const std::string& tex)
{
	SetHandleTextures(tex, tex, tex);
}

void Slider::SetHandleTextureJSON(const std::string& path)
{
	if (!path.empty())
		SetHandleTextures(path, path, path);
}

void Slider::SetHandleTextures(const std::string& idle, const std::string& hover, const std::string& pressed)
{
	m_handlePathIdle = idle;
	m_handlePathHover = hover.empty() ? idle : hover;
	m_handlePathPressed = pressed.empty() ? idle : pressed;

	auto& rm = ResourceManager::GetInstance();
	if (!m_handlePathIdle.empty()) m_handleTexIdle = rm.GetTextureAndOffset(m_handlePathIdle);
	if (!m_handlePathHover.empty()) m_handleTexHover = rm.GetTextureAndOffset(m_handlePathHover);
	if (!m_handlePathPressed.empty()) m_handleTexPressed = rm.GetTextureAndOffset(m_handlePathPressed);

	UpdateHandleRect();
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
	auto color = m_colorIdle;
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

	float handleX = m_UIRect.left + t * (m_UIRect.right - m_UIRect.left);
	float handleY = (m_UIRect.top + m_UIRect.bottom) * 0.5f;

	DirectX::XMFLOAT2 handlePos = Renderer::GetInstance().ToUIPosition({ handleX, handleY });

	const auto& handlePair =
		(m_handleState == HandleState::Pressed) ? m_handleTexPressed :
		(m_handleState == HandleState::Hover) ? m_handleTexHover :
		m_handleTexIdle;
	auto handleTex = handlePair.first;
	auto handleOff = handlePair.second;

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
	const DirectX::XMFLOAT2 windowPos = Renderer::GetInstance().ToScreenPosition(GetWorldPosition());

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

	UpdateHandleRect();
}

void Slider::UpdateHandleRect()
{
	const float range = (m_max - m_min);
	float t = (range != 0.0f) ? (m_value - m_min) / range : 0.0f;
	t = std::clamp(t, 0.0f, 1.0f);

	float handleX = m_UIRect.left + t * (m_UIRect.right - m_UIRect.left);
	float handleY = (m_UIRect.top + m_UIRect.bottom) * 0.5f;

	DirectX::XMFLOAT2 offset = m_handleTexIdle.second;
	offset.x = std::max({ offset.x, m_handleTexHover.second.x, m_handleTexPressed.second.x });
	offset.y = std::max({ offset.y, m_handleTexHover.second.y, m_handleTexPressed.second.y });

	offset.x *= m_scale;
	offset.y *= m_scale;

	m_handleRect =
	{
		(LONG)(handleX - offset.x),
		(LONG)(handleY - offset.y),
		(LONG)(handleX + offset.x),
		(LONG)(handleY + offset.y)
	};
}

bool Slider::CheckInput(const POINT& mousePos, bool isMousePressed)
{
	if (!IsActuallyActive())
	{
		m_dragging = false;
		m_handleState = HandleState::Idle;
		return false;
	}

	UpdateHandleRect();
	const bool isOverHandle = PtInRect(&m_handleRect, mousePos);

	if (!isMousePressed)
	{
		m_dragging = false;
		m_handleState = isOverHandle ? HandleState::Hover : HandleState::Idle;
		return false;
	}

	if (!m_dragging)
	{
		if (!PtInRect(&m_UIRect, mousePos) && !isOverHandle)
			return false;

		m_dragging = true;
	}

	m_handleState = HandleState::Pressed;

	float t =
		float(mousePos.x - m_UIRect.left) /
		float(m_UIRect.right - m_UIRect.left);

	t = std::clamp(t, 0.0f, 1.0f);
	m_value = m_min + t * (m_max - m_min);
	UpdateHandleRect();

	return true;
}


nlohmann::json Slider::Serialize() const
{
	nlohmann::json data = UIBase::Serialize();

	data["min"] = m_min;
	data["max"] = m_max;
	data["value"] = m_value;
	data["handlePathIdle"] = m_handlePathIdle;
	data["handlePathHover"] = m_handlePathHover;
	data["handlePathPressed"] = m_handlePathPressed;
	data["textureHandle"] = m_handlePathIdle;
	if (!m_onValueChangedActionKey.empty())
		data["actionKey"] = m_onValueChangedActionKey;

	return data;
}

void Slider::Deserialize(const nlohmann::json& jsonData)
{
	UIBase::Deserialize(jsonData);

	if (jsonData.contains("min")) m_min = jsonData["min"];
	if (jsonData.contains("max")) m_max = jsonData["max"];
	if (jsonData.contains("value")) {
		float val = jsonData["value"];
		SetValue(val);
	}

	std::string handleIdle = jsonData.value("handlePathIdle", "");
	std::string handleHover = jsonData.value("handlePathHover", "");
	std::string handlePressed = jsonData.value("handlePathPressed", "");

	if (!handleIdle.empty() || !handleHover.empty() || !handlePressed.empty())
	{
		if (handleIdle.empty()) handleIdle = handleHover.empty() ? handlePressed : handleHover;
		SetHandleTextures(handleIdle, handleHover, handlePressed);
	}
	else if (jsonData.contains("textureHandle"))
	{
		std::string handlePath = jsonData["textureHandle"];
		if (!handlePath.empty()) { SetHandleTexture(handlePath); }
	}

	if (jsonData.contains("actionKey")) {m_onValueChangedActionKey = jsonData["actionKey"];	}
}
