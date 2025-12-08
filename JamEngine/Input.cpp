#include "pch.h"

#include "Input.h"

#include "Application.h"
#include "Event.h"
#include "Events.h"

namespace jam
{

constexpr Int32 k_wheelDelta = 120;

struct InputContext
{
    void OnMouseDown(const MouseDownEvent& _event)
    {
        Int32 vKey = _event.GetVKey();
        JAM_ASSERT(vKey >= 0 && vKey < 256, "Invalid virtual key code: {}", vKey);
        bKeyDown[vKey] = true;
    }

    void OnMouseUp(const MouseUpEvent& _event)
    {
        Int32 vKey = _event.GetVKey();
        JAM_ASSERT(vKey >= 0 && vKey < 256, "Invalid virtual key code: {}", vKey);
        bKeyDown[vKey] = false;
    }

    void OnMouseWheel(const MouseWheelEvent& _event)
    {
        mouseDeltaWheel = static_cast<float>(_event.GetDelta()) / k_wheelDelta;
    }

    void OnKeyDown(const KeyDownEvent& _event)
    {
        Int32 vKey = _event.GetVKey();
        JAM_ASSERT(vKey >= 0 && vKey < 256, "Invalid virtual key code: {}", vKey);
        bKeyDown[vKey] = true;
    }

    void OnKeyUp(const KeyUpEvent& _event)
    {
        Int32 vKey = _event.GetVKey();
        JAM_ASSERT(vKey >= 0 && vKey < 256, "Invalid virtual key code: {}", vKey);
        bKeyDown[vKey] = false;
    }

    void OnWindowLostFocusEvent(const WindowLostFocusEvent& _event)
    {
        std::memset(bKeyDown, 0, sizeof(bKeyDown));
    }

    bool bPrevKeyDown[256] = { false };
    bool bKeyDown[256]     = { false };

    Int32 mousePrevX = 0;
    Int32 mousePrevY = 0;
    Int32 mouseX     = 0;
    Int32 mouseY     = 0;

    float mouseDeltaWheel = 0;
    bool  bCursorVisible  = true;
    bool  bCursorGrabbed  = false;

    EventDispatcher eventDispatcher;
}* g_pInputCtx;

}   // namespace jam

namespace jam
{

void Input::Init()
{
    JAM_ASSERT(!g_pInputCtx, "InputContext is already initialized.");
    g_pInputCtx = JAM_NEW(InputContext);

    EventDispatcher& dispatcher = g_pInputCtx->eventDispatcher;
    dispatcher.AddListener<KeyDownEvent>(g_pInputCtx, &InputContext::OnKeyDown);
    dispatcher.AddListener<KeyUpEvent>(g_pInputCtx, &InputContext::OnKeyUp);
    dispatcher.AddListener<MouseDownEvent>(g_pInputCtx, &InputContext::OnMouseDown);
    dispatcher.AddListener<MouseUpEvent>(g_pInputCtx, &InputContext::OnMouseUp);
    dispatcher.AddListener<MouseWheelEvent>(g_pInputCtx, &InputContext::OnMouseWheel);
    dispatcher.AddListener<WindowLostFocusEvent>(g_pInputCtx, &InputContext::OnWindowLostFocusEvent);
}

void Input::Shutdown()
{
    JAM_ASSERT(g_pInputCtx, "InputContext is not initialized.");
    JAM_DELETE(g_pInputCtx);
}

void Input::Update()
{
    const Application&     app = Application::GetInstance();
    const WindowsPlatform& win = app.GetPlatform();
    const auto [x, y]          = win.GetClientPos();

    std::memcpy(g_pInputCtx->bPrevKeyDown, g_pInputCtx->bKeyDown, sizeof(g_pInputCtx->bKeyDown));

    g_pInputCtx->mousePrevX      = g_pInputCtx->mouseX;
    g_pInputCtx->mousePrevY      = g_pInputCtx->mouseY;
    g_pInputCtx->mouseDeltaWheel = 0;

    POINT p;
    ::GetCursorPos(&p);
    g_pInputCtx->mouseX = p.x - x;
    g_pInputCtx->mouseY = p.y - y;
}

void Input::OnEvent(Event& _event)
{
    const EventDispatcher& dispatcher = g_pInputCtx->eventDispatcher;
    dispatcher.Dispatch(_event);
}

bool Input::IsKeyDown(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return g_pInputCtx->bKeyDown[index];
}

bool Input::IsKeyPressed(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return g_pInputCtx->bKeyDown[index] && !g_pInputCtx->bPrevKeyDown[index];
}

bool Input::IsKeyHold(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return g_pInputCtx->bKeyDown[index] && g_pInputCtx->bPrevKeyDown[index];
}

bool Input::IsKeyUp(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return !g_pInputCtx->bKeyDown[index];
}

bool Input::IsKeyReleased(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return !g_pInputCtx->bKeyDown[index] && g_pInputCtx->bPrevKeyDown[index];
}

bool Input::IsKeyAway(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return !g_pInputCtx->bKeyDown[index] && !g_pInputCtx->bPrevKeyDown[index];
}

bool Input::IsMouseDown(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return g_pInputCtx->bKeyDown[index];
}

bool Input::IsMousePressed(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return g_pInputCtx->bKeyDown[index] && !g_pInputCtx->bPrevKeyDown[index];
}

bool Input::IsMouseHold(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return g_pInputCtx->bKeyDown[index] && g_pInputCtx->bPrevKeyDown[index];
}

bool Input::IsMouseUp(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return !g_pInputCtx->bKeyDown[index];
}

bool Input::IsMouseReleased(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return !g_pInputCtx->bKeyDown[index] && g_pInputCtx->bPrevKeyDown[index];
}

bool Input::IsMouseAway(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return !g_pInputCtx->bKeyDown[index] && !g_pInputCtx->bPrevKeyDown[index];
}

Vec2i Input::GetMousePos() noexcept
{
    return { g_pInputCtx->mouseX, g_pInputCtx->mouseY };
}

float Input::GetMouseDeltaWheel() noexcept
{
    return g_pInputCtx->mouseDeltaWheel;
}

Vec2i Input::GetMouseDeltaPos() noexcept
{
    const Int32 deltaX = g_pInputCtx->mouseX - g_pInputCtx->mousePrevX;
    const Int32 deltaY = g_pInputCtx->mouseY - g_pInputCtx->mousePrevY;
    return { deltaX, deltaY };
}

void Input::SetMouseVisibility(const bool _bVisible) noexcept
{
    if (_bVisible)
    {
        while (::ShowCursor(TRUE) < 0)
            ;
    }
    else
    {
        while (::ShowCursor(FALSE) >= 0)
            ;
    }

    g_pInputCtx->bCursorVisible = _bVisible;
}

void Input::GrabMouseCursor(const bool _bEnable) noexcept
{
    g_pInputCtx->bCursorGrabbed = _bEnable;

    if (_bEnable)
    {
        const Application&     app = Application::GetInstance();
        const WindowsPlatform& win = app.GetPlatform();
        const auto [cliX, cliY]    = win.GetClientPos();
        const auto [width, height] = win.GetClientResolution();

        RECT clipRect;
        clipRect.left   = cliX;
        clipRect.top    = cliY;
        clipRect.right  = cliX + width;
        clipRect.bottom = cliY + height;
        ::ClipCursor(&clipRect);
    }
    else
    {
        ::ClipCursor(nullptr);
    }
}

bool Input::IsMouseVisible() noexcept
{
    return g_pInputCtx->bCursorVisible;
}

bool Input::IsMouseCursorGrabbed() noexcept
{
    return g_pInputCtx->bCursorGrabbed;
}

void Input::SetMousePos(const Int32 _x, const Int32 _y) noexcept
{
    const Application&     app = Application::GetInstance();
    const WindowsPlatform& win = app.GetPlatform();
    const auto [cliX, cliY]    = win.GetClientPos();
    const Int32 screenX        = cliX + _x;
    const Int32 screenY        = cliY + _y;

    ::SetCursorPos(screenX, screenY);
    g_pInputCtx->mouseX = _x;
    g_pInputCtx->mouseY = _y;
}

}   // namespace jam
