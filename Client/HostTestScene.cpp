/// HostTestScene.cpp¿« Ω√¿€
#include "stdafx.h"
#include "HostTestScene.h"

#include "YdmTestObject.h"
#include "CoordinateSystemObject.h"
//#include "BlenderCameraObject.h"
#include "TestCameraObject.h"
#include "CamRotObject.h"

using namespace std;
using namespace DirectX;

REGISTER_TYPE(HostTestScene)

void HostTestScene::Initialize()
{
	CreateRootGameObject("YdmTestObject");
	CreateRootGameObject("CoordinateSystemObject");
}

GameObjectBase* HostTestScene::CreateCameraObject()
{
	return CreateRootGameObject<CamRotObject>("CamRotObject")->CreateChildGameObject<TestCameraObject>();
}