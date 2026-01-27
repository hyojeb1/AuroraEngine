/// TaehyeonTestScene.h의 시작
/// 반드시 깔끔한 씬배치를 해내겠음
#pragma once
#include "SceneBase.h"


class TaehyeonTestScene : public SceneBase
{
public:
	TaehyeonTestScene() = default;
	~TaehyeonTestScene() override = default;
	TaehyeonTestScene(const TaehyeonTestScene&) = default;
	TaehyeonTestScene& operator=(const TaehyeonTestScene&) = default;
	TaehyeonTestScene(TaehyeonTestScene&&) = default;
	TaehyeonTestScene& operator=(TaehyeonTestScene&&) = default;

protected:
	virtual void Initialize() override;
	virtual void Update() override;
};
/// TaehyeonTestScene.h의 끝
