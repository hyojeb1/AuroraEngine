#pragma once
#include "SceneBase.h"

class TitleScene : public SceneBase
{
public:
	TitleScene() = default;
	~TitleScene() override = default;
	TitleScene(const TitleScene&) = default;
	TitleScene& operator=(const TitleScene&) = default;
	TitleScene(TitleScene&&) = default;
	TitleScene& operator=(TitleScene&&) = default;

private:
	void Initialize() override;
	void Update() override;
};