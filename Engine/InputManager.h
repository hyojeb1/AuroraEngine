#pragma once
#include "Singleton.h"
#include "KeyCode.h"

class InputManager : public Singleton<InputManager>
{
	std::array<bool, 256> m_keyState = {};
	std::array<bool, 256> m_keyDownState = {};
	std::array<bool, 256> m_keyUpState = {};

	POINT m_mousePos = { 0, 0 };
	POINT m_mouseDelta = { 0, 0 };
	int m_wheelDelta = 0;

public:
    friend class Singleton<InputManager>;

    using UINT = unsigned int;
    using WPARAM = std::uintptr_t;
    using LPARAM = std::intptr_t;

	void Initialize();

    void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void EndFrame();

    bool GetKeyDown(KeyCode key) const;
    bool GetKey(KeyCode key) const;
    bool GetKeyUp(KeyCode key) const;

    const POINT& GetMousePosition() const { return m_mousePos; }
    const POINT& GetMouseDelta() const { return m_mouseDelta; }
    int GetMouseWheel() const { return m_wheelDelta; }

private:
	InputManager() = default;

    void RegisterRawDevice();
	void ProcessRawInput(LPARAM lParam);
	void ProcessRawKeyboard(const RAWKEYBOARD& keyboard);
	void ProcessRawMouse(const RAWMOUSE& mouse);

    int MapKeyCodeToVKey(KeyCode key) const;
};

/// InputManager.h의 끝