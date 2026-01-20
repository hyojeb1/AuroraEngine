///GunObject.h의 시작
#pragma once
#include "GameObjectBase.h"

class FSMComponent;

class GunObject : public GameObjectBase
{
public:
	GunObject() = default;
	~GunObject() override = default;
	GunObject(const GunObject&) = default;
	GunObject& operator=(const GunObject&) = default;
	GunObject(GunObject&&) = default;
	GunObject& operator=(GunObject&&) = default;

	void Fire();

private:
	void Initialize() override;
	void Update() override;

	FSMComponent* m_fsmComponent = nullptr;
};