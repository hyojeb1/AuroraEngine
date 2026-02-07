#include "stdafx.h"
#include "UIBase.h"
#include "Slider.h"

#include "ResourceManager.h"
#include "Renderer.h"

Slider::Slider()
{
	SetTextureAndOffset("UI_IDLE.png"); 
	SetHandleTexture("UI_IDLE.png");    
	m_scale = 0.1f;
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

void Slider::SetHandleTextureJSON(const std::string& path)
{
	m_handleTexturePath = path;
	if (!path.empty())
		m_handleTex = ResourceManager::GetInstance().GetTextureAndOffset(path);
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

nlohmann::json Slider::Serialize() const
{
	nlohmann::json data = UIBase::Serialize();

	data["min"] = m_min;
	data["max"] = m_max;
	data["value"] = m_value;
	data["textureHandle"] = m_handleTexturePath;
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

	if (jsonData.contains("textureHandle")) {
		std::string handlePath = jsonData["textureHandle"];
		if (!handlePath.empty()) { SetHandleTexture(handlePath);}
	}

	if (jsonData.contains("actionKey")) {m_onValueChangedActionKey = jsonData["actionKey"];	}
}