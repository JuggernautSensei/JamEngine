#pragma once

namespace jam
{

class Event;

enum class eKey : int
{
    // 숫자 키
    Key0 = '0',
    Key1 = '1',
    Key2 = '2',
    Key3 = '3',
    Key4 = '4',
    Key5 = '5',
    Key6 = '6',
    Key7 = '7',
    Key8 = '8',
    Key9 = '9',

    // 영문
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

    // 펑션 키
    F1  = VK_F1,
    F2  = VK_F2,
    F3  = VK_F3,
    F4  = VK_F4,
    F5  = VK_F5,
    F6  = VK_F6,
    F7  = VK_F7,
    F8  = VK_F8,
    F9  = VK_F9,
    F10 = VK_F10,
    F11 = VK_F11,
    F12 = VK_F12,

    // 화살표
    LeftArrow  = VK_LEFT,
    UpArrow    = VK_UP,
    RightArrow = VK_RIGHT,
    DownArrow  = VK_DOWN,

    // 특수 키
    Escape     = VK_ESCAPE,
    Space      = VK_SPACE,
    Tab        = VK_TAB,
    Backspace  = VK_BACK,
    Enter      = VK_RETURN,
    Shift      = VK_SHIFT,
    Control    = VK_CONTROL,
    Alt        = VK_MENU,
    CapsLock   = VK_CAPITAL,
    NumLock    = VK_NUMLOCK,
    ScrollLock = VK_SCROLL,

    // 넘패드
    NumPad0  = VK_NUMPAD0,
    NumPad1  = VK_NUMPAD1,
    NumPad2  = VK_NUMPAD2,
    NumPad3  = VK_NUMPAD3,
    NumPad4  = VK_NUMPAD4,
    NumPad5  = VK_NUMPAD5,
    NumPad6  = VK_NUMPAD6,
    NumPad7  = VK_NUMPAD7,
    NumPad8  = VK_NUMPAD8,
    NumPad9  = VK_NUMPAD9,
    Multiply = VK_MULTIPLY,
    Add      = VK_ADD,
    Subtract = VK_SUBTRACT,
    Decimal  = VK_DECIMAL,
    Divide   = VK_DIVIDE,

    // 기타
    PrintScreen = VK_PRINT,
    Pause       = VK_PAUSE,
    Insert      = VK_INSERT,
    Delete      = VK_DELETE,
    Home        = VK_HOME,
    End         = VK_END,
    PageUp      = VK_PRIOR,
    PageDown    = VK_NEXT,

    // 사용하지 않음
    None = 0x00
};

enum class eMouse
{
    LeftButton   = VK_LBUTTON,
    RightButton  = VK_RBUTTON,
    MiddleButton = VK_MBUTTON,
    XButton1     = VK_XBUTTON1,
    XButton2     = VK_XBUTTON2,

    None = 0x00
};

/// @brief 입력 처리 클래스
namespace input
{
    void Init();
    void Update();
    void OnEvent(Event& _event);

    JAM_NODISCARD bool IsKeyDown(eKey _key) noexcept;
    JAM_NODISCARD bool IsKeyPressed(eKey _key) noexcept;
    JAM_NODISCARD bool IsKeyHold(eKey _key) noexcept;

    JAM_NODISCARD bool IsKeyUp(eKey _key) noexcept;
    JAM_NODISCARD bool IsKeyReleased(eKey _key) noexcept;
    JAM_NODISCARD bool IsKeyAway(eKey _key) noexcept;

    JAM_NODISCARD bool IsMouseDown(eMouse _mouse) noexcept;
    JAM_NODISCARD bool IsMousePressed(eMouse _mouse) noexcept;
    JAM_NODISCARD bool IsMouseHold(eMouse _mouse) noexcept;

    JAM_NODISCARD bool IsMouseUp(eMouse _mouse) noexcept;
    JAM_NODISCARD bool IsMouseReleased(eMouse _mouse) noexcept;
    JAM_NODISCARD bool IsMouseAway(eMouse _mouse) noexcept;

    JAM_NODISCARD std::pair<Int32, Int32> GetMousePos() noexcept;
    JAM_NODISCARD Int32                   GetMouseDeltaWheel() noexcept;
    JAM_NODISCARD std::pair<Int32, Int32> GetMouseDeltaPos() noexcept;
};   // namespace input

}   // namespace jam