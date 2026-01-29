// TestScene.h의 시작
#pragma once
#include "SceneBase.h"

class EndingScene : public SceneBase
{
public:
	EndingScene() = default;
	~EndingScene() override = default;
	EndingScene(const EndingScene&) = default;
	EndingScene& operator=(const EndingScene&) = default;
	EndingScene(EndingScene&&) = default;
	EndingScene& operator=(EndingScene&&) = default;

private:
	void Initialize() override;
	void Update() override;
};