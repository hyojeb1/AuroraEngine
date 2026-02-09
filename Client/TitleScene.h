#pragma once
#include "SceneBase.h"

class TitleScene : public SceneBase
{
	class Panel* optionPanel = nullptr;
	class Panel* creditPanel = nullptr;
	class Panel* Titles = nullptr;
	class Panel* Title_letterrbox_down = nullptr;
	class Panel* Title_letterrbox_up = nullptr;

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