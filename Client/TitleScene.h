#pragma once
#include "SceneBase.h"

namespace {
	constexpr float kTime4MovingPanel = 2.0f;
}

class TitleScene : public SceneBase
{
	class Panel* optionPanel = nullptr;
	class Panel* creditPanel = nullptr;
	class Panel* Titles = nullptr;
	class Panel* Title_letterrbox_down = nullptr;
	class Panel* Title_letterrbox_up = nullptr;

	float m_time4MovingPanel = 0;

private:
	void Initialize() override;
	void Update() override;
	void BindUIActions() override;
	
	void MovingPanel(float dt);

public:
	TitleScene() = default;
	~TitleScene() override = default;
	TitleScene(const TitleScene&) = default;
	TitleScene& operator=(const TitleScene&) = default;
	TitleScene(TitleScene&&) = default;
	TitleScene& operator=(TitleScene&&) = default;
};