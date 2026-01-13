#include "stdafx.h"
#include "HostTestScene.h"

#include "YdmTestObject.h"
#include "TestCameraObject.h"
#include "CamRotObject.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(HostTestScene)

void HostTestScene::Initialize()
{
	CreateRootGameObject("YdmTestObject");
	CreateRootGameObject<CamRotObject>("CamRotObject")->CreateChildGameObject<TestCameraObject>();
}