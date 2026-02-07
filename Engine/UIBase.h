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

	//직렬화 
	virtual nlohmann::json Serialize() const;
	virtual void Deserialize(const nlohmann::json& data);
	static UIBase* CreateFactory(const std::string& typeName);
	virtual std::string GetTypeName() const = 0;

protected:
	virtual void UpdateRect() = 0;
	std::string m_name = "UI";
	RECT m_UIRect = {};
	bool m_isActive = true;
	UIBase* m_parent = nullptr;
	DirectX::XMFLOAT2 m_localPosition = {};
	float m_scale = 1.0f;
	DirectX::XMVECTOR m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float m_depth = 0.0f;

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureIdle = {};
};
