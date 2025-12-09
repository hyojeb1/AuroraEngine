#include "stdafx.h"

#include "WindowManager.h"
#include "Renderer.h"

#include "TestScene.h"

int main()
{
	WindowManager windowManager;
	windowManager.Initialize(L"Window");

	Renderer renderer;
	renderer.Initialize(windowManager.GetHWnd());

	TestScene testScene;
	testScene.Initialize(&renderer);

	while (windowManager.ProcessMessages())
	{
		testScene.Update(0.005f);
		testScene.TransformGameObjects();
		testScene.Render();
	};
}