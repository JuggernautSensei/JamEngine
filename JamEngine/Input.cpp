#include "pch.h"

#include "Input.h"

#include "Event.h"
#include "Events.h"

namespace jam
{

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

    void OnMouseMove(const MouseMoveEvent& _event)
    {
        mouseX = _event.GetX();
        mouseY = _event.GetY();
    }

    void OnMouseWheel(const MouseWheelEvent& _event)
    {
        mouseDeltaWheel = _event.GetDelta();
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

    bool bPrevKeyDown[256] = { false };
    bool bKeyDown[256]     = { false };

    Int32 mousePrevX      = 0;
    Int32 mousePrevY      = 0;
    Int32 mouseX          = 0;
    Int32 mouseY          = 0;
    Int32 mouseDeltaWheel = 0;

    EventDispatcher eventDispatcher;
} g_inputContext;

}   // namespace jam

namespace jam
{

void Input::Init()
{
    EventDispatcher& dispatcher = g_inputContext.eventDispatcher;
    dispatcher.AddListener<KeyDownEvent>(&g_inputContext, &InputContext::OnKeyDown);
    dispatcher.AddListener<KeyUpEvent>(&g_inputContext, &InputContext::OnKeyUp);
    dispatcher.AddListener<MouseDownEvent>(&g_inputContext, &InputContext::OnMouseDown);
    dispatcher.AddListener<MouseUpEvent>(&g_inputContext, &InputContext::OnMouseUp);
    dispatcher.AddListener<MouseMoveEvent>(&g_inputContext, &InputContext::OnMouseMove);
    dispatcher.AddListener<MouseWheelEvent>(&g_inputContext, &InputContext::OnMouseWheel);
}

void Input::Update()
{
    std::memcpy(g_inputContext.bPrevKeyDown, g_inputContext.bKeyDown, sizeof(g_inputContext.bKeyDown));

    g_inputContext.mousePrevX      = g_inputContext.mouseX;
    g_inputContext.mousePrevY      = g_inputContext.mouseY;
    g_inputContext.mouseDeltaWheel = 0;
}

void Input::OnEvent(Event& _event)
{
    const EventDispatcher& dispatcher = g_inputContext.eventDispatcher;
    dispatcher.Dispatch(_event);
}

bool Input::IsKeyDown(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return g_inputContext.bKeyDown[index];
}

bool Input::IsKeyPressed(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return g_inputContext.bKeyDown[index] && !g_inputContext.bPrevKeyDown[index];
}

bool Input::IsKeyHold(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return g_inputContext.bKeyDown[index] && g_inputContext.bPrevKeyDown[index];
}

bool Input::IsKeyUp(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return !g_inputContext.bKeyDown[index];
}

bool Input::IsKeyReleased(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return !g_inputContext.bKeyDown[index] && g_inputContext.bPrevKeyDown[index];
}

bool Input::IsKeyAway(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return !g_inputContext.bKeyDown[index] && !g_inputContext.bPrevKeyDown[index];
}

bool Input::IsMouseDown(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return g_inputContext.bKeyDown[index];
}

bool Input::IsMousePressed(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return g_inputContext.bKeyDown[index] && !g_inputContext.bPrevKeyDown[index];
}

bool Input::IsMouseHold(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return g_inputContext.bKeyDown[index] && g_inputContext.bPrevKeyDown[index];
}

bool Input::IsMouseUp(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return !g_inputContext.bKeyDown[index] && g_inputContext.bPrevKeyDown[index];
}

bool Input::IsMouseReleased(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return !g_inputContext.bKeyDown[index] && g_inputContext.bPrevKeyDown[index];
}

bool Input::IsMouseAway(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return !g_inputContext.bKeyDown[index] && !g_inputContext.bPrevKeyDown[index];
}

WindowPos Input::GetMousePos() noexcept
{
    return { g_inputContext.mouseX, g_inputContext.mouseY };
}

Int32 Input::GetMouseDeltaWheel() noexcept
{
    return static_cast<Int32>(g_inputContext.mouseDeltaWheel);
}

WindowPos Input::GetMouseDeltaPos() noexcept
{
    const Int32 deltaX = g_inputContext.mouseX - g_inputContext.mousePrevX;
    const Int32 deltaY = g_inputContext.mouseY - g_inputContext.mousePrevY;
    return { deltaX, deltaY };
}

}   // namespace jam
