#pragma once
#include "GameObjectBase.h"

class DebugCamera : public GameObjectBase
{
	float m_sensitivity = 0.1f;
	float m_moveSpeed = 10.0f;

public:
    DebugCamera() = default;

    void Initialize() override;
    void Update() override;
	void RenderImGui() override;
};