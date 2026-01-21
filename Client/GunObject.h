///GunObject.h의 시작
#pragma once
#include "GameObjectBase.h"

class GunObject : public GameObjectBase
{
public:
	void Fire();

	GunObject() = default;
	~GunObject() override = default;
	GunObject(const GunObject&) = default;
	GunObject& operator=(const GunObject&) = default;
	GunObject(GunObject&&) = default;
	GunObject& operator=(GunObject&&) = default;

private:
	void Initialize() override;
	void Update() override;
};