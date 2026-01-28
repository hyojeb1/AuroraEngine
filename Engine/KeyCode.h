/// KeyCode.h의 시작
/// https://github.com/DragonT-iger/DTEngine/blob/main/DTEngine/KeyCode.h
/// 
#pragma once

enum class KeyCode
{

    None = 0,

    // 마우스
    MouseLeft = 1,
    MouseRight = 2,
    MouseMiddle = 3,

    // 제어 키
    Control,
    Shift,
    Alt,
    Space,
    Enter,
    Escape,
    Tab,
    Backspace,
    Delete,

    // 방향 키
    Left,
    Right,
    Up,
    Down,

    // 문자
    A = 'A',
    B = 'B',
    C = 'C',
    D = 'D',
    E = 'E',
    F = 'F',
    G = 'G',
    H = 'H',
    I = 'I',
    J = 'J',
    K = 'K',
    L = 'L',
    M = 'M',
    N = 'N',
    O = 'O',
    P = 'P',
    Q = 'Q',
    R = 'R',
    S = 'S',
    T = 'T',
    U = 'U',
    V = 'V',
    W = 'W',
    X = 'X',
    Y = 'Y',
    Z = 'Z',

    // 숫자
    Num0 = '0',
    Num1 = '1',
    Num2 = '2',
    Num3 = '3',
    Num4 = '4',
    Num5 = '5',
    Num6 = '6',
    Num7 = '7',
    Num8 = '8',
    Num9 = '9',

	F1 = VK_F1,
	F2 = VK_F2,
	F3 = VK_F3,
	F4 = VK_F4,
	F5 = VK_F5,
	F6 = VK_F6,
	F7 = VK_F7,
	F8 = VK_F8,
	F9 = VK_F9,
	F10 = VK_F10,
	F11 = VK_F11,
	F12 = VK_F12,

	LeftBracket = VK_OEM_4, // [
	RightBracket = VK_OEM_6, // ]
};