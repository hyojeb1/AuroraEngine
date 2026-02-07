#pragma once

//enum X list

class UIBase
{
public:
	UIBase() = default;
	virtual ~UIBase() = default;
	UIBase(const UIBase&) = delete;
	UIBase& operator=(const UIBase&) = delete;

	virtual void RenderUI(class Renderer& renderer) = 0;

	// =========================================================
	// Getter/Setter
	// =========================================================

	// --- Active ---
	void SetActive(bool isActive) { m_isActive = isActive; }
	bool GetActive() const { return m_isActive; }

	bool IsActuallyActive() const {
		if (!m_isActive) return false;
		if (m_parent) return m_parent->IsActuallyActive();
		return true;
	}

	// --- Parent ---
	void SetParent(UIBase* parent) { m_parent = parent; UpdateRect(); }
	UIBase* GetParent() const { return m_parent; }

	// --- Position ---
	void SetLocalPosition(const DirectX::XMFLOAT2& pos) { m_localPosition = pos; UpdateRect(); }
	const DirectX::XMFLOAT2& GetLocalPosition() const { return m_localPosition; }

	DirectX::XMFLOAT2 GetWorldPosition() const {
		if (m_parent) {
			DirectX::XMFLOAT2 parentPos = m_parent->GetWorldPosition();
			return DirectX::XMFLOAT2(parentPos.x + m_localPosition.x, parentPos.y + m_localPosition.y);
		}
		return m_localPosition;
	}

	// --- Scale ---
	void SetScale(float scale) { m_scale = scale; UpdateRect(); }
	float GetScale() const { return m_scale; }

	// --- Color ---
	void SetColor(const DirectX::XMVECTOR& color) { m_color = color; }
	const DirectX::XMVECTOR& GetColor() const { return m_color; }

	// --- Depth ---
	void SetDepth(float depth) { m_depth = depth; }
	float GetDepth() const { return m_depth; }

	// --- Name ---
	void SetName(const std::string& name) { m_name = name; }
	const std::string& GetName() const { return m_name; }

	// --- Rect (Read Only) ---
	const RECT& GetRect() const { return m_UIRect; }

	// --- Texture & Offset ---
	void SetTextureAndOffset(const std::string& idle);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture() const { return m_textureIdle.first; }
	const DirectX::XMFLOAT2& GetTextureOffset() const { return m_textureIdle.second; }

	void SetTextureRaw(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture, const DirectX::XMFLOAT2& offset = { 0,0 }) {
		m_textureIdle.first = texture;
		m_textureIdle.second = offset;
	}


	// =========================================================
	// Serialization
	// =========================================================
	virtual void OnResize() { UpdateRect(); }

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
