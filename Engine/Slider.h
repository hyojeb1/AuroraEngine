#pragma once

class Slider : public UIBase
{
public:
	Slider();
	~Slider() override = default;

	void RenderUI(Renderer& renderer) override;
	bool CheckInput(const POINT& mousePos, bool isMousePressed);

	void SetRange(float min, float max);
	void SetValue(float newValue);
	float& GetValue() { return m_value; }
	float& GetMin() { return m_min; }
	float& GetMax() { return m_max; }


	void SetHandleTexture(const std::string& tex);
	void SetHandleTextures(const std::string& idle, const std::string& hover, const std::string& pressed);
	void SetHandleTextureJSON(const std::string& path);

	void AddListener(std::function<void(float)>);
	void NotifyValueChanged();

	std::string GetTypeName() const override { return "Slider"; }

	void SetActionKey(const std::string& key) { m_onValueChangedActionKey = key; }
	std::string GetActionKey() const { return m_onValueChangedActionKey; }

	nlohmann::json Serialize() const override;
	void Deserialize(const nlohmann::json& jsonData) override;

public:
	std::string m_handlePathIdle = "UI_IDLE.png";
	std::string m_handlePathHover = "UI_IDLE.png";
	std::string m_handlePathPressed = "UI_IDLE.png";

private:
	enum class HandleState
	{
		Idle,
		Hover,
		Pressed
	};

	void UpdateRect() override;
	void UpdateHandleRect();

	float m_min = 0.0f;
	float m_max = 1.0f;
	float m_value = 0.5f;

	bool m_dragging = false;

	HandleState m_handleState = HandleState::Idle;
	RECT m_handleRect = {};

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_handleTexIdle{};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_handleTexHover{};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_handleTexPressed{};

	std::vector<std::function<void(float)>> listeners;
	std::string m_onValueChangedActionKey = "";

};
