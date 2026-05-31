#include "pch.h"

#include "Input.h"

#include "Event.h"
#include "Events.h"
#include "WindowsPlatform.h"

namespace Jam
{

void Input::CreateInstance()
{
    JAM_ASSERT(!s_pInstance, "Input instance already exists.");
    s_pInstance = new Input;
}

void Input::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

Input& Input::GetInstance()
{
    JAM_ASSERT(s_pInstance, "Input instance does not exist.");
    return *s_pInstance;
}

void Input::Update()
{
    // update key info
    static_assert(sizeof(m_bKeyDown) == sizeof(m_bPrevKeyDown), "Key state arrays size mismatch.");
    std::memcpy(m_bPrevKeyDown.data(), m_bKeyDown.data(), sizeof(m_bKeyDown));

    // update mouse info
    m_mousePrevX      = m_mouseX;
    m_mousePrevY      = m_mouseY;
    m_mouseDeltaWheel = 0;

    // get cursor pos
    WindowsPlatform& win    = GetWindowsPlatform();
    const Vec2i      cliPos = win.GetClientPos();

    POINT p = {};
    ::GetCursorPos(&p);
    m_mouseX = p.x - cliPos.x;
    m_mouseY = p.y - cliPos.y;
}

void Input::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<KeyDownEvent>(this, &Input::OnKeyDown_);
    dispatcher.Dispatch<KeyUpEvent>(this, &Input::OnKeyUp_);
    dispatcher.Dispatch<MouseDownEvent>(this, &Input::OnMouseDown_);
    dispatcher.Dispatch<MouseUpEvent>(this, &Input::OnMouseUp_);
    dispatcher.Dispatch<MouseWheelEvent>(this, &Input::OnMouseWheel_);
    dispatcher.Dispatch<WindowLostFocusEvent>(this, &Input::OnWindowLostFocusEvent_);
}

bool Input::IsKeyDown(const eKey _key) noexcept
{
    const Int32 index = AsUnderlying(_key);
    return m_bKeyDown[index];
}

bool Input::IsKeyPressed(const eKey _key) noexcept
{
    const Int32 index = AsUnderlying(_key);
    return m_bKeyDown[index] && !m_bPrevKeyDown[index];
}

bool Input::IsKeyHold(const eKey _key) noexcept
{
    const Int32 index = AsUnderlying(_key);
    return m_bKeyDown[index] && m_bPrevKeyDown[index];
}

bool Input::IsKeyUp(const eKey _key) noexcept
{
    const Int32 index = AsUnderlying(_key);
    return !m_bKeyDown[index];
}

bool Input::IsKeyReleased(const eKey _key) noexcept
{
    const Int32 index = AsUnderlying(_key);
    return !m_bKeyDown[index] && m_bPrevKeyDown[index];
}

bool Input::IsKeyAway(const eKey _key) noexcept
{
    const Int32 index = AsUnderlying(_key);
    return !m_bKeyDown[index] && !m_bPrevKeyDown[index];
}

bool Input::IsMouseDown(const eMouse _mouse) noexcept
{
    const Int32 index = AsUnderlying(_mouse);
    return m_bKeyDown[index];
}

bool Input::IsMousePressed(const eMouse _mouse) noexcept
{
    const Int32 index = AsUnderlying(_mouse);
    return m_bKeyDown[index] && !m_bPrevKeyDown[index];
}

bool Input::IsMouseHold(const eMouse _mouse) noexcept
{
    const Int32 index = AsUnderlying(_mouse);
    return m_bKeyDown[index] && m_bPrevKeyDown[index];
}

bool Input::IsMouseUp(const eMouse _mouse) noexcept
{
    const Int32 index = AsUnderlying(_mouse);
    return !m_bKeyDown[index];
}

bool Input::IsMouseReleased(const eMouse _mouse) noexcept
{
    const Int32 index = AsUnderlying(_mouse);
    return !m_bKeyDown[index] && m_bPrevKeyDown[index];
}

bool Input::IsMouseAway(const eMouse _mouse) noexcept
{
    const Int32 index = AsUnderlying(_mouse);
    return !m_bKeyDown[index] && !m_bPrevKeyDown[index];
}

Vec2i Input::GetMousePos() const noexcept
{
    return { m_mouseX, m_mouseY };
}

Vec2i Input::GetMouseDeltaPos() const noexcept
{
    const Int32 deltaX = m_mouseX - m_mousePrevX;
    const Int32 deltaY = m_mouseY - m_mousePrevY;
    return { deltaX, deltaY };
}

float Input::GetMouseDeltaWheel() const noexcept
{
    return m_mouseDeltaWheel;
}

void Input::SetMousePos(
    const Int32 _x,
    const Int32 _y) noexcept
{
    WindowsPlatform& win     = GetWindowsPlatform();
    const Vec2i      cliPos  = win.GetClientPos();
    const Int32      screenX = cliPos.x + _x;
    const Int32      screenY = cliPos.y + _y;

    ::SetCursorPos(screenX, screenY);
    m_mouseX = _x;
    m_mouseY = _y;
}

void Input::SetMouseVisibility(const bool _bVisible) noexcept
{
    if (_bVisible)
    {
        while (::ShowCursor(TRUE) < 0)
        {
            JAM_NOOP;
        }
    }
    else
    {
        while (::ShowCursor(FALSE) >= 0)
        {
            JAM_NOOP;
        }
    }

    m_bCursorVisiblity = _bVisible;
}

bool Input::IsMouseVisible() const noexcept
{
    return m_bCursorVisiblity;
}

void Input::OnMouseDown_(
    const MouseDownEvent& _event)
{
    const Int32 vKey = _event.GetVKey();
    m_bKeyDown[vKey] = true;
}

void Input::OnKeyDown_(
    const KeyDownEvent& _event)
{
    const Int32 vKey = _event.GetVKey();
    m_bKeyDown[vKey] = true;
}

void Input::OnKeyUp_(
    const KeyUpEvent& _event)
{
    const Int32 vKey = _event.GetVKey();
    m_bKeyDown[vKey] = false;
}

void Input::OnMouseUp_(
    const MouseUpEvent& _event)
{
    const Int32 vKey = _event.GetVKey();
    m_bKeyDown[vKey] = false;
}

void Input::OnMouseWheel_(
    const MouseWheelEvent& _event)
{
    static_assert(kWheelSensitive != 0, "Wheel sensitivity cannot be zero.");
    m_mouseDeltaWheel = static_cast<float>(_event.GetDelta()) / static_cast<float>(kWheelSensitive);
}

void Input::OnWindowLostFocusEvent_(
    const WindowLostFocusEvent&)
{
    m_bKeyDown.fill(false);
}

}   // namespace Jam
