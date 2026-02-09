#include "stdafx.h"
#include "Text.h"

#include "Renderer.h"
#include "ResourceManager.h"

using namespace DirectX;

Text::Text()
{
	UpdateRect();
}

void Text::RenderUI(Renderer& renderer)
{
	if (!IsActuallyActive())
		return;

	std::wstring text = GetTextWide();
	std::wstring fontName = GetFontNameWide();
	if (text.empty())
		return;

	auto pos = GetWorldPosition();
	auto color = m_colorIdle;
	auto scale = GetFinalScale();
	auto depth = m_depth;

	renderer.UI_RENDER_FUNCTIONS().emplace_back(
		[text, fontName, pos, color, scale, depth]() {
			Renderer::GetInstance().RenderTextUIPosition(
				text.c_str(), pos, depth, color, scale, fontName
			);
		});
}

void Text::SetText(const std::string& text)
{
	m_textUtf8 = text;
	UpdateRect();
}

void Text::SetFontName(const std::string& fontName)
{
	m_fontNameUtf8 = fontName;
	UpdateRect();
}

nlohmann::json Text::Serialize() const
{
	nlohmann::json data = UIBase::Serialize();
	data["text"] = m_textUtf8;
	data["fontName"] = m_fontNameUtf8;

	XMFLOAT4 color;
	XMStoreFloat4(&color, m_colorIdle);
	data["textColor"] = { color.x, color.y, color.z, color.w };

	return data;
}

void Text::Deserialize(const nlohmann::json& data)
{
	UIBase::Deserialize(data);

	if (data.contains("text")) m_textUtf8 = data["text"];
	if (data.contains("fontName")) m_fontNameUtf8 = data["fontName"];

	if (data.contains("textColor")) {
		XMFLOAT4 color = {};
		color.x = data["textColor"][0];
		color.y = data["textColor"][1];
		color.z = data["textColor"][2];
		color.w = data["textColor"][3];
		m_colorIdle = XMLoadFloat4(&color);
	}

	UpdateRect();
}

void Text::UpdateRect()
{
	std::wstring text = GetTextWide();
	std::wstring fontName = GetFontNameWide();

	if (text.empty()) {
		m_UIRect = {};
		return;
	}

	auto* font = ResourceManager::GetInstance().GetSpriteFont(fontName);
	if (!font) {
		m_UIRect = {};
		return;
	}

	XMVECTOR size = font->MeasureString(text.c_str());
	XMFLOAT2 sizePixels{};
	XMStoreFloat2(&sizePixels, size);

	sizePixels.x *= GetFinalScale();
	sizePixels.y *= GetFinalScale();

	const XMFLOAT2 screenPos = Renderer::GetInstance().ToScreenPosition(GetWorldPosition());

	m_UIRect =
	{
		static_cast<LONG>(screenPos.x),
		static_cast<LONG>(screenPos.y),
		static_cast<LONG>(screenPos.x + sizePixels.x),
		static_cast<LONG>(screenPos.y + sizePixels.y)
	};
}

std::wstring Text::GetTextWide() const
{
	return Utf8ToWide(m_textUtf8);
}

std::wstring Text::GetFontNameWide() const
{
	return Utf8ToWide(m_fontNameUtf8);
}

std::wstring Text::Utf8ToWide(const std::string& text)
{
	if (text.empty()) return L"";

	const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
	if (length <= 0) return L"";

	std::wstring result(static_cast<size_t>(length - 1), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
	return result;
}

std::string Text::WideToUtf8(const std::wstring& text)
{
	if (text.empty()) return {};

	const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (length <= 0) return {};

	std::string result(static_cast<size_t>(length - 1), '\0');
	WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), length, nullptr, nullptr);
	return result;
}