#include "stdafx.h" 
#include "InputManager.h"
#include "WindowManager.h"

using namespace std;

void InputManager::Initialize()
{
	m_keyState.fill(false);
	m_keyDownState.fill(false);
	m_keyUpState.fill(false);

    RegisterRawDevice();
}

void InputManager::RegisterRawDevice()
{
	HWND hWnd = WindowManager::GetInstance().GetHWnd();

	const array<RAWINPUTDEVICE, 2> rid =
	{
		RAWINPUTDEVICE
		{
			.usUsagePage = 0x01, // HID_USAGE_PAGE_GENERIC
			.usUsage = 0x06, // HID_USAGE_GENERIC_KEYBOARD
			.dwFlags = 0,
			.hwndTarget = hWnd
		},
		RAWINPUTDEVICE
		{
			.usUsagePage = 0x01, // HID_USAGE_PAGE_GENERIC
			.usUsage = 0x02, // HID_USAGE_GENERIC_MOUSE
			.dwFlags = 0,
			.hwndTarget = hWnd
		}
	};

	if (!RegisterRawInputDevices(rid.data(), 2, sizeof(RAWINPUTDEVICE))) DWORD error = GetLastError();
}

void InputManager::EndFrame()
{
	m_keyDownState.fill(false);
	m_keyUpState.fill(false);
	m_wheelDelta = 0;
	m_mouseDelta = { 0, 0 };
}

void InputManager::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INPUT:
	{
		ProcessRawInput(lParam);
		break;
	}

	case WM_MOUSEWHEEL:
		m_wheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
		break;
	}
}

void InputManager::ProcessRawInput(LPARAM lParam)
{
	UINT size = 0;
	GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

	vector<BYTE> buffer(size);
	if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) return;

	RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());

	if (raw->header.dwType == RIM_TYPEKEYBOARD) ProcessRawKeyboard(raw->data.keyboard);
	else if (raw->header.dwType == RIM_TYPEMOUSE) ProcessRawMouse(raw->data.mouse);
}

void InputManager::ProcessRawKeyboard(const RAWKEYBOARD& keyboard)
{
	unsigned int vKey = keyboard.VKey;
	if (vKey >= 256) return;

	bool isKeyDown = !(keyboard.Flags & RI_KEY_BREAK);

	if (isKeyDown)
	{
		if (!m_keyState[vKey]) m_keyDownState[vKey] = true;
		m_keyState[vKey] = true;
	}
	else
	{
		m_keyUpState[vKey] = true;
		m_keyState[vKey] = false;
	}
}

void InputManager::ProcessRawMouse(const RAWMOUSE& mouse)
{
	if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
	{
		if (!m_keyState[VK_LBUTTON]) m_keyDownState[VK_LBUTTON] = true;
		m_keyState[VK_LBUTTON] = true;
	}
	if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
	{
		m_keyUpState[VK_LBUTTON] = true;
		m_keyState[VK_LBUTTON] = false;
	}
	if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
	{
		if (!m_keyState[VK_RBUTTON]) m_keyDownState[VK_RBUTTON] = true;
		m_keyState[VK_RBUTTON] = true;
	}
	if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
	{
		m_keyUpState[VK_RBUTTON] = true;
		m_keyState[VK_RBUTTON] = false;
	}
	if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
	{
		if (!m_keyState[VK_MBUTTON]) m_keyDownState[VK_MBUTTON] = true;
		m_keyState[VK_MBUTTON] = true;
	}
	if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
	{
		m_keyUpState[VK_MBUTTON] = true;
		m_keyState[VK_MBUTTON] = false;
	}

	m_mouseDelta.x += static_cast<int>(mouse.lLastX);
	m_mouseDelta.y += static_cast<int>(mouse.lLastY);

	GetCursorPos(&m_mousePos);
	ScreenToClient(WindowManager::GetInstance().GetHWnd(), &m_screenMousePos);
}

bool InputManager::GetKeyDown(KeyCode key) const
{
	int vKey = MapKeyCodeToVKey(key);

	return (vKey < 256) ? m_keyDownState[vKey] : false;
}

bool InputManager::GetKey(KeyCode key) const
{
	int vKey = MapKeyCodeToVKey(key);

	return (vKey < 256) ? m_keyState[vKey] : false;
}

bool InputManager::GetKeyUp(KeyCode key) const
{
	int vKey = MapKeyCodeToVKey(key);

	return (vKey < 256) ? m_keyUpState[vKey] : false;
}

int InputManager::MapKeyCodeToVKey(KeyCode key) const
{
	if ((key >= KeyCode::A && key <= KeyCode::Z) || (key >= KeyCode::Num0 && key <= KeyCode::Num9)) return static_cast<int>(key);

	switch (key)
	{
	case KeyCode::MouseLeft:    return VK_LBUTTON;
	case KeyCode::MouseRight:   return VK_RBUTTON;
	case KeyCode::MouseMiddle:  return VK_MBUTTON;
	case KeyCode::Control:      return VK_CONTROL;
	case KeyCode::Shift:        return VK_SHIFT;
	case KeyCode::Alt:          return VK_MENU;
	case KeyCode::Space:        return VK_SPACE;
	case KeyCode::Enter:        return VK_RETURN;
	case KeyCode::Escape:       return VK_ESCAPE;
	case KeyCode::Tab:          return VK_TAB;
	case KeyCode::Backspace:    return VK_BACK;
	case KeyCode::Delete:       return VK_DELETE;
	case KeyCode::Left:         return VK_LEFT;
	case KeyCode::Right:        return VK_RIGHT;
	case KeyCode::Up:           return VK_UP;
	case KeyCode::Down:         return VK_DOWN;
	case KeyCode::F1:           return VK_F1;
	case KeyCode::F2:           return VK_F2;
	case KeyCode::F3:           return VK_F3;
	case KeyCode::F4:           return VK_F4;
	case KeyCode::F5:           return VK_F5;
	case KeyCode::F6:           return VK_F6;
	case KeyCode::F7:           return VK_F7;
	case KeyCode::F8:           return VK_F8;
	case KeyCode::F9:           return VK_F9;
	case KeyCode::F10:          return VK_F10;
	case KeyCode::F11:          return VK_F11;
	case KeyCode::F12:          return VK_F12;
	default:                    return 0;
	}
}