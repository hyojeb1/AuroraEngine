#pragma once
#include "GameObjectBase.h"

class CamRotObject : public GameObjectBase
{
	float m_ySensitivity = 0.1f;

public:
	CamRotObject() = default;
	~CamRotObject() override = default;
	CamRotObject(const CamRotObject&) = default;
	CamRotObject& operator=(const CamRotObject&) = default;
	CamRotObject(CamRotObject&&) = default;
	CamRotObject& operator=(CamRotObject&&) = default;

	float GetSensitivity() const { return m_ySensitivity; }
	void SetSensitivity(float sensitivity) { m_ySensitivity = sensitivity; }

private:
	void Initialize() override;
	void Update() override;
};