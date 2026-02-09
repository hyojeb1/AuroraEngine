#pragma once

class UIBase
{
	friend class SceneBase; // 에디터에서 private 변수 접근 허용

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
	void SetScaleIdle(float scale) { m_scaleIdle = scale; UpdateRect(); }
	float GetScaleIdle() const { return m_scaleIdle; }

	// --- Color ---
	void SetColorIdle(const DirectX::XMVECTOR& color) { m_colorIdle = color; }
	const DirectX::XMVECTOR& GetColorIdle() const { return m_colorIdle; }

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
	virtual void OnResize(std::pair<float, float> res) { SetScale(res.first); }//UpdateRect(); }

	virtual nlohmann::json Serialize() const;
	virtual void Deserialize(const nlohmann::json& data);

	static UIBase* CreateFactory(const std::string& typeName);
	virtual std::string GetTypeName() const = 0;

	// =========================================================
	// Animation
	// =========================================================
	void Update();
	
	void GetTextureResolution(int& outWidth, int& outHeight) const;
	int GetMaxFrames() const;
	void ClampFrame();
	void UpdateFlipBookRect();

protected:
	virtual void UpdateRect() = 0;
	
	std::string m_name = "UI";
	RECT m_UIRect = {};
	bool m_isActive = true;
	UIBase* m_parent = nullptr;
	DirectX::XMFLOAT2 m_localPosition = {};
	float m_scale = 1.0f;
	float m_scaleIdle = 1.0f;
	DirectX::XMVECTOR m_colorIdle = { 1.0f, 1.0f, 1.0f, 1.0f };
	float m_depth = 0.0f;

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureIdle = {};
	std::string m_pathIdle = "UI_IDLE.png";

	// Animation
	RECT m_UIAnimationRect = {};
	int m_rows = 1;
	int m_columns = 1;
	int m_startFrame = 0;
	int m_endFrame = 1;
	float m_framesPerSecond = 8.0f;
	bool m_loop = true;
	bool m_playing = true;
	bool auto_play_ = true;
	bool destroy_on_finish_ = false;
	float playback_speed_ = 1.0f;
	int m_currentFrame = 0;
	float m_accumulatedTime = 0.0f;
};
