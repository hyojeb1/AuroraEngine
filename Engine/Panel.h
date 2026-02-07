#pragma once

class Panel : public UIBase
{
public:
	Panel();
	void OnResize() override;
	void RenderUI(class Renderer& renderer) override;
	void AddChild(std::unique_ptr<UIBase> child) { child->SetParent(this); m_children.emplace_back(std::move(child)); }

	std::string GetTypeName() const override { return "Panel"; }

private:
	void UpdateRect() override;

	std::vector<std::unique_ptr<UIBase>> m_children;
};
