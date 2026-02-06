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

	std::string ToString(ButtonState type)
	{
		switch (type)
		{
		case ButtonState::Idle: return "Idle";
		case ButtonState::Hoverd:  return "Hoverd";
		case ButtonState::Pressed:  return "Pressed";
		case ButtonState::Clicked: return "Clicked";
		}
	}

	ButtonState m_ButtonState = ButtonState::Idle;

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureHoverd = {};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_texturePressed = {};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureClicked = {};

	std::function<void()> m_onClick = nullptr;
public:
	Button() = default;
	~Button() = default;
	void SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed); //overroad
	void SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed, const std::string& clicked);

	void SetOnClick(const std::function<void()>& onClick) { m_onClick = onClick; }

	void RenderUI(class Renderer& renderer) override;
	bool CheckInput(const POINT& mousePosition, bool isMouseClicked, bool isMousePressed);

private:
	void UpdateRect() override;
};