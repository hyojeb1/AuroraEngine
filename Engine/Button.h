#pragma once

class Button : public UIBase
{
	enum class ButtonState
	{
		Idle,
		Hoverd,
		Pressed,
		Clicked
	};

	ButtonState m_ButtonState = ButtonState::Idle;

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureHoverd = {};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_texturePressed = {};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureClicked = {};

	std::string m_pathHover = "";
	std::string m_pathPressed = "";
	std::string m_pathClicked = "";

	//DirectX::XMFLOAT2 m_offsetHover = { 0.f, 0.f };
	//DirectX::XMFLOAT2 m_offsetPressed = { 0.f, 0.f };
	//DirectX::XMFLOAT2 m_offsetClicked = { 0.f, 0.f };

	std::function<void()> m_onClick = nullptr;
	std::string m_onClickActionKey = "";

public:
	Button();
	~Button() = default;

	std::string ToString(ButtonState type)
	{
		switch (type) {
		case ButtonState::Idle: return "Idle";
		case ButtonState::Hoverd:  return "Hoverd";
		case ButtonState::Pressed:  return "Pressed";
		case ButtonState::Clicked: return "Clicked";
		}
	}

	void SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed); //overroad
	void SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed, const std::string& clicked);

	void SetOnClick(const std::function<void()>& onClick) { m_onClick = onClick; }

	void RenderUI(class Renderer& renderer) override;
	bool CheckInput(const POINT& mousePosition, bool isMouseClicked, bool isMousePressed);

	std::string GetTypeName() const override { return "Button"; }

	void SetActionKey(const std::string& key) { m_onClickActionKey = key; }
	std::string GetActionKey() const { return m_onClickActionKey; }

	void SetButtonTextures(const std::string& idle, const std::string& hover, const std::string& pressed, const std::string& clicked);

	//void SetHoverOffset(const DirectX::XMFLOAT2& offset) { m_offsetHover = offset; }
	//void SetPressedOffset(const DirectX::XMFLOAT2& offset) { m_offsetPressed = offset; }
	//void SetClickedOffset(const DirectX::XMFLOAT2& offset) { m_offsetClicked = offset; }

	DirectX::XMFLOAT2& GetHoverOffset()		{ return  m_textureHoverd.second; }
	DirectX::XMFLOAT2& GetPressedOffset()	{ return  m_texturePressed.second; }
	DirectX::XMFLOAT2& GetClickedOffset()	{ return  m_textureClicked.second; }

	nlohmann::json Serialize() const override;
	void Deserialize(const nlohmann::json& jsonData) override;

	const std::string GetHoverPath() { return m_pathHover;	};
	const std::string GetPressedPath(){ return m_pathPressed; };
	const std::string GetClickedPath(){ return m_pathClicked; };

private:
	void UpdateRect() override;
};