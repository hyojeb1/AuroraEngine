#pragma once

#include "Singleton.h"

class SoundManager : public Singleton<SoundManager>
{
public:

	friend class Singleton<SoundManager>;

	void Initialize();
	void Update();
	void Finalize();

};

