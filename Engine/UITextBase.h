#pragma once

#include "UIBase.h"

class UITextBase : public UIBase
{
public:
	UITextBase();
	~UITextBase() override = default;

	void RenderUI(class Renderer& renderer) override;

	void SetText(const std::string& text);
	const std::string& GetText() const { return m_textUtf8; }

	void SetFontName(const std::string& fontName);
	const std::string& GetFontName() const { return m_fontNameUtf8; }

	nlohmann::json Serialize() const override;
	void Deserialize(const nlohmann::json& data) override;

	std::string GetTypeName() const override { return "Text"; }

private:
	void UpdateRect() override;

	std::wstring GetTextWide() const;
	std::wstring GetFontNameWide() const;

	static std::wstring Utf8ToWide(const std::string& text);
	static std::string WideToUtf8(const std::wstring& text);

	std::string m_textUtf8 = "Text";
	std::string m_fontNameUtf8 = "Gugi";
};