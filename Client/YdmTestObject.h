///YdmTestObject.h의 시작
#pragma once
#include "GameObjectBase.h"

class YdmTestObject : public GameObjectBase
{
public:
	YdmTestObject() = default;
	~YdmTestObject() override = default;
	YdmTestObject(const YdmTestObject&) = default;
	YdmTestObject& operator=(const YdmTestObject&) = default;
	YdmTestObject(YdmTestObject&&) = default;
	YdmTestObject& operator=(YdmTestObject&&) = default;

private:
	void Initialize() override;
	void Update() override;
};

///YdmTestObject.h의 끝