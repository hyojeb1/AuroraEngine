#pragma once

class UIBase
{
public:
	UIBase() = default;
	virtual ~UIBase() = default;

	UIBase(const UIBase&) = delete;
	UIBase& operator=(const UIBase&) = delete;

	virtual void RenderUI(class Renderer& renderer) = 0;

	void SetActive(bool isActive) { m_isActive = isActive; }
	bool GetActive() { return m_isActive; }

	void SetLocalPosition(const DirectX::XMFLOAT2& pos) { m_localPosition = pos; UpdateRect(); }
	DirectX::XMFLOAT2 GetWorldPosition() const
	{
		if (m_parent)
			return	{
			m_parent->GetWorldPosition().x + m_localPosition.x,
			m_parent->GetWorldPosition().y + m_localPosition.y
		};

		return m_localPosition;
	}
	void SetParent(UIBase* parent) { m_parent = parent;	UpdateRect(); }
	void SetScale(float scale) { m_scale = scale; UpdateRect(); }
	void SetColor(const DirectX::XMVECTOR& color) { m_color = color; }
	void SetDepth(float depth) { m_depth = depth; }
	void SetTextureAndOffset(const std::string& idle);

	bool IsActuallyActive() const
	{
		if (!m_isActive)
			return false;

		if (m_parent)
			return m_parent->IsActuallyActive();

		return true;
	}

	virtual void OnResize() { UpdateRect(); }

protected:
	virtual void UpdateRect() = 0;
	RECT m_UIRect = {};
	bool m_isActive = true;
	UIBase* m_parent = nullptr;
	DirectX::XMFLOAT2 m_localPosition = {};
	float m_scale = 1.0f;
	DirectX::XMVECTOR m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float m_depth = 0.0f;

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureIdle = {};
};

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

class Panel : public UIBase
{
public:
	void OnResize() override;
	void RenderUI(class Renderer& renderer) override;
	void AddChild(std::unique_ptr<UIBase> child) { child->SetParent(this); m_children.emplace_back(std::move(child)); }
private:
	void UpdateRect() override;

	std::vector<std::unique_ptr<UIBase>> m_children;
};

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