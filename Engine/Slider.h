#pragma once

class Slider : public UIBase
{
public:
	Slider() = default;
	~Slider() override = default;

	void RenderUI(Renderer& renderer) override;
	bool CheckInput(const POINT& mousePos, bool isMousePressed);

	void SetRange(float min, float max);
	void SetValue(float newValue);
	float GetValue() const { return m_value; }

	void SetHandleTexture(const std::string& tex);
	//void SetFillTexture(const std::string& tex) { m_fillTexture = ResourceManager::GetInstance().GetTextureAndOffset(tex); }
	
	void AddListener(std::function<void(float)>);
	void NotifyValueChanged();

	std::string GetTypeName() const override { return "Slider"; }

private:
	void UpdateRect() override;

	float m_min = 0.0f;
	float m_max = 1.0f;
	float m_value = 0.5f;

	bool m_dragging = false;

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_handleTex{};
	//std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_fillTexture{};

	std::vector<std::function<void(float)>> listeners;
};