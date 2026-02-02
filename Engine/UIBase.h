////
////class UIBase
////{
////protected:
////	bool m_isActive = true;
////
////	void SetActive(bool isActive) { m_isActive = isActive; }
////};
////
//
//class Button// : public UIBase
//{
//	enum class ButtonState
//	{
//		Idle,
//		Hoverd,
//		Pressed,
//		Clicked
//	};
//
//	std::string ToString(ButtonState type)
//	{
//		switch (type)
//		{
//		case ButtonState::Idle: return "Idle";
//		case ButtonState::Hoverd:  return "Hoverd";
//		case ButtonState::Pressed:  return "Pressed";
//		case ButtonState::Clicked: return "Clicked";
//		}
//	}
//
//	bool m_isActive = true;
//
//	ButtonState m_ButtonState;
//
//	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureIdle = {};
//	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureHoverd = {};
//	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_texturePressed = {};
//	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_textureClicked = {};
//
//	DirectX::XMFLOAT2 m_UIPosition = {};
//	float m_scale = 1.0f;
//	DirectX::XMVECTOR m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
//	float m_depth = 0.0f;
//
//	RECT m_buttonRect = {};
//
//	std::function<void()> m_onClick = nullptr;
//
//public:
//	void SetActive(bool isActive) { m_isActive = isActive; }
//	void SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed); //overroad
//	void SetTextureAndOffset(const std::string& idle, const std::string& hoverd, const std::string& pressed, const std::string& clicked);
//	void SetUIPosition(const DirectX::XMFLOAT2& position) { m_UIPosition = position; UpdateRect(); }
//	void SetScale(float scale) { m_scale = scale; UpdateRect(); }
//	void SetColor(const DirectX::XMVECTOR& color) { m_color = color; }
//	void SetDepth(float depth) { m_depth = depth; }
//	void SetOnClick(const std::function<void()>& onClick) { m_onClick = onClick; }
//
//	void RenderButton(class Renderer& renderer);
//	void CheckInput(const POINT& mousePosition, bool isMouseClicked, bool isMousePressed);
//
//private:
//	void UpdateRect();
//};