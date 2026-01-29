#pragma once


class CrossHairAndNode : public ComponentBase
{
public:
	bool NeedsFixedUpdate() const override { return false; }
	bool NeedsUpdate() const override { return true; }
	bool NeedsRender() const override { return true; }

	void Initialize() override;
	void Update() override;
	void Render() override;
	void RenderImGui() override;
	void Finalize() override;

	nlohmann::json Serialize() override;
	void Deserialize(const nlohmann::json& jsonData) override;

private:
	void GenerateNode();
	
	void RenderCrossHair(class Renderer& renderer);
	void RenderUINode(class Renderer& renderer);

	void ResizeMiddleCH(class InputManager& input, float delta);

	std::deque<float> m_UINode = {};

	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_crosshairTextureAndOffset = {};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_RhythmLineTextureAndOffset = {};
	std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> m_NodeAndOffset = {};

	float m_CrossHairSize;
	float m_NodeStartSize;
	float m_NodeEndSize;

	float m_NodeStart;
	float m_NodeEnd;

	float m_linePos;
	float m_lineScl;

	std::unordered_map<std::string, std::vector<std::pair<float, float>>>* m_NodeDataPtr = nullptr;


};

