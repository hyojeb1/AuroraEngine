#pragma once
#include "GameObjectBase.h"

class Enemy : public GameObjectBase
{
public:
	Enemy() = default;
	~Enemy() override = default;
	Enemy(const Enemy&) = default;
	Enemy& operator=(const Enemy&) = default;
	Enemy(Enemy&&) = default;
	Enemy& operator=(Enemy&&) = default;

private:
	void Initialize() override;
};