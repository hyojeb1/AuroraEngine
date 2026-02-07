#pragma once

class Button : public UIBase
{
	friend class SceneBase;

	enum class ButtonState
	{
		Idle,
		Hoverd,
		Pressed,
		Clicked
	};

	ButtonState m_ButtonState = ButtonState::Idle;

	// --- [텍스처 리소스] ---
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureHoverd = {};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_texturePressed = {};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureClicked = {};

	// --- [직렬화 변수들] ---
	// 1. 텍스처 경로
	std::string m_pathHover = "UI_Hovered.png";
	std::string m_pathPressed = "UI_Pressed.png";
	std::string m_pathClicked = "UI_Clicked.png";

	// 2. 스케일 (기본 Scale에 곱해지는 배율)
	float m_scaleHover = 1.0f;
	float m_scalePressed = 0.95f; // 눌렀을 때 살짝 작아지는 느낌
	float m_scaleClicked = 1.0f;

	// 3. 컬러 (틴트)
	DirectX::XMVECTOR m_colorHover = { 0.9f, 0.9f, 0.9f, 1.0f };   // 살짝 어둡게
	DirectX::XMVECTOR m_colorPressed = { 0.7f, 0.7f, 0.7f, 1.0f }; // 더 어둡게
	DirectX::XMVECTOR m_colorClicked = { 1.0f, 1.0f, 1.0f, 1.0f };

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

	nlohmann::json Serialize() const override;
	void Deserialize(const nlohmann::json& jsonData) override;

private:
	void UpdateRect() override;
};