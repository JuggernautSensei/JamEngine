#pragma once

namespace Jam
{
class KeyDownEvent;
class KeyUpEvent;
class MouseUpEvent;
class MouseWheelEvent;
class WindowLostFocusEvent;
class MouseDownEvent;

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

class Input
{
public:
    Input(const Input&)            = delete;
    Input(Input&&)                 = delete;
    Input& operator=(const Input&) = delete;
    Input& operator=(Input&&)      = delete;

    [[nodiscard]] static Input& GetInstance();

    [[nodiscard]] bool IsKeyDown(eKey _key) noexcept;
    [[nodiscard]] bool IsKeyPressed(eKey _key) noexcept;
    [[nodiscard]] bool IsKeyHold(eKey _key) noexcept;

    [[nodiscard]] bool IsKeyUp(eKey _key) noexcept;
    [[nodiscard]] bool IsKeyReleased(eKey _key) noexcept;
    [[nodiscard]] bool IsKeyAway(eKey _key) noexcept;

    [[nodiscard]] bool IsMouseDown(eMouse _mouse) noexcept;
    [[nodiscard]] bool IsMousePressed(eMouse _mouse) noexcept;
    [[nodiscard]] bool IsMouseHold(eMouse _mouse) noexcept;

    [[nodiscard]] bool IsMouseUp(eMouse _mouse) noexcept;
    [[nodiscard]] bool IsMouseReleased(eMouse _mouse) noexcept;
    [[nodiscard]] bool IsMouseAway(eMouse _mouse) noexcept;

    [[nodiscard]] Vec2i GetMousePos() const noexcept;
    [[nodiscard]] Vec2i GetMouseDeltaPos() const noexcept;
    [[nodiscard]] float GetMouseDeltaWheel() const noexcept;

    void               SetMousePos(Int32 _x, Int32 _y) noexcept;   // window coords
    void               SetMouseVisibility(bool _bVisible) noexcept;
    [[nodiscard]] bool IsMouseVisible() const noexcept;

private:
    Input() = default;

    static void CreateInstance();
    static void DestroyInstance();
    void        Update();
    void        OnEvent(Event& _event);

    void OnMouseDown_(const MouseDownEvent& _event);
    void OnKeyDown_(const KeyDownEvent& _event);
    void OnKeyUp_(const KeyUpEvent& _event);
    void OnMouseUp_(const MouseUpEvent& _event);
    void OnMouseWheel_(const MouseWheelEvent& _event);
    void OnWindowLostFocusEvent_(const WindowLostFocusEvent& _event);

    Array<bool, 256> m_bPrevKeyDown = {};
    Array<bool, 256> m_bKeyDown     = {};

    Int32 m_mousePrevX = 0;
    Int32 m_mousePrevY = 0;
    Int32 m_mouseX     = 0;
    Int32 m_mouseY     = 0;

    float m_mouseDeltaWheel  = 0;
    bool  m_bCursorVisiblity = true;

    static constexpr Int32 kWheelSensitive = 120;
    inline static Input*   s_pInstance      = nullptr;
    friend class Application;
};

[[nodiscard]] inline Input& GetInput()
{
    return Input::GetInstance();
}

}   // namespace Jam

