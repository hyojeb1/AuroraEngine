#pragma once
#include "Singleton.h"

class LUT : public Singleton<LUT>
{
public:
//private:
	//get set은 나중에, 지금은 퍼블릭
	static constexpr int LUT_SIZE = 16;
	static constexpr int TOTAL_SIZE = LUT_SIZE * LUT_SIZE * LUT_SIZE;
	std::array<uint32_t, TOTAL_SIZE> volume_data_;

public:
	friend class Singleton<LUT>;
	void Initialize() 
	{ 
		
	}

	
};