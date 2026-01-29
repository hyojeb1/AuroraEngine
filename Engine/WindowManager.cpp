// WindowManager.h의 시작
#include "stdafx.h"
#include "WindowManager.h"

#include "Renderer.h"
#include "InputManager.h"
#include "RNG.h"

using namespace std;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowManager::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) return true;

	InputManager::GetInstance().HandleMessage(message, wParam, lParam);

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);

		return 0;

	case WM_SIZE:
		Renderer::GetInstance().Resize(static_cast<UINT>(LOWORD(lParam)), static_cast<UINT>(HIWORD(lParam)));

		#ifdef NDEBUG
		if (GetForegroundWindow() == hWnd)
		{
			RECT clipRect;
			GetClientRect(hWnd, &clipRect);
			MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&clipRect), 2);
			ClipCursor(&clipRect);
		}
		#endif

		return 0;

	case WM_ACTIVATE:
		#ifdef NDEBUG
		if (LOWORD(wParam) != WA_INACTIVE)
		{
			RECT clipRect;
			GetClientRect(hWnd, &clipRect);
			MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&clipRect), 2);
			ClipCursor(&clipRect);
		}
		else ClipCursor(nullptr);
		#endif

		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void WindowManager::Initialize(const wchar_t* windowTitle, int width, int height, const wchar_t* className)
{
	// 한글 출력 설정
	SetConsoleOutputCP(CP_UTF8);

	const WNDCLASS wc =
	{
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WindowProc,
		.hInstance = m_hInstance,
		.lpszClassName = className
	};
	if (!RegisterClass(&wc))
	{
		cerr << "윈도우 클래스 등록 실패. 에러 코드: " << hex << GetLastError() << endl;
		exit(EXIT_FAILURE);
	}

	RECT windowRect = { 0, 0, width, height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
	const int adjustedWidth = windowRect.right - windowRect.left;
	const int adjustedHeight = windowRect.bottom - windowRect.top;

	m_hWnd = CreateWindow
	(
		className,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		adjustedWidth,
		adjustedHeight,
		nullptr,
		nullptr,
		m_hInstance,
		nullptr
	);
	if (!m_hWnd)
	{
		cerr << "윈도우 생성 실패. 에러 코드: " << hex << GetLastError() << endl;
		exit(EXIT_FAILURE);
	}

	// ImGui Win32 초기화
	ImGui_ImplWin32_Init(m_hWnd);
	// 렌더러 초기화
	Renderer::GetInstance().Initialize();
	// 인풋매니저 초기화
	InputManager::GetInstance().Initialize();
	// RNG 초기화
	RNG::GetInstance().Initialize();

	ShowWindow(m_hWnd, SW_SHOW);
}

bool WindowManager::ProcessMessages()
{
	MSG msg = {};
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// ImGui Win32 새 프레임 시작
	ImGui_ImplWin32_NewFrame();

	return true;
}

void WindowManager::Finalize()
{
	// 렌더러 종료
	Renderer::GetInstance().Finalize();
	
	// 인풋매니저 종료 // 따로 필요 없음

	// ImGui Win32 종료
	ImGui_ImplWin32_Shutdown();

	// 윈도우 파괴 및 클래스 등록 해제
	DestroyWindow(m_hWnd);
	m_hWnd = nullptr;
	UnregisterClass(L"EngineWindowClass", m_hInstance);
}

RECT WindowManager::GetClientPosRect() const
{
	RECT rect;

	GetClientRect(m_hWnd, &rect);
	ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&rect.left));

	return rect;
}