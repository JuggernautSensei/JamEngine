#include "pch.h"

#include "Input.h"

#include "Event.h"
#include "Events.h"
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

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
};

static InputContext g_ctx;

////////////////////////////////////////////////////////////

namespace jam::input
{

void Init()
{
    EventDispatcher& dispatcher = g_ctx.eventDispatcher;
    dispatcher.AddListener<KeyDownEvent>(&g_ctx, &InputContext::OnKeyDown);
    dispatcher.AddListener<KeyUpEvent>(&g_ctx, &InputContext::OnKeyUp);
    dispatcher.AddListener<MouseDownEvent>(&g_ctx, &InputContext::OnMouseDown);
    dispatcher.AddListener<MouseUpEvent>(&g_ctx, &InputContext::OnMouseUp);
    dispatcher.AddListener<MouseMoveEvent>(&g_ctx, &InputContext::OnMouseMove);
    dispatcher.AddListener<MouseWheelEvent>(&g_ctx, &InputContext::OnMouseWheel);
}

void Update()
{
    std::memcpy(g_ctx.bPrevKeyDown, g_ctx.bKeyDown, sizeof(g_ctx.bKeyDown));

    g_ctx.mousePrevX      = g_ctx.mouseX;
    g_ctx.mousePrevY      = g_ctx.mouseY;
    g_ctx.mouseDeltaWheel = 0;
}

void OnEvent(Event& _event)
{
    const EventDispatcher& dispatcher = g_ctx.eventDispatcher;
    dispatcher.Dispatch(_event);
}

bool IsKeyDown(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return g_ctx.bKeyDown[index];
}

bool IsKeyPressed(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return g_ctx.bKeyDown[index] && !g_ctx.bPrevKeyDown[index];
}

bool IsKeyHold(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return g_ctx.bKeyDown[index] && g_ctx.bPrevKeyDown[index];
}

bool IsKeyUp(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return !g_ctx.bKeyDown[index];
}

bool IsKeyReleased(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return !g_ctx.bKeyDown[index] && g_ctx.bPrevKeyDown[index];
}

bool IsKeyAway(const eKey _key) noexcept
{
    int index = EnumToInt(_key);
    return !g_ctx.bKeyDown[index] && !g_ctx.bPrevKeyDown[index];
}

bool IsMouseDown(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return g_ctx.bKeyDown[index];
}

bool IsMousePressed(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return g_ctx.bKeyDown[index] && !g_ctx.bPrevKeyDown[index];
}

bool IsMouseHold(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return g_ctx.bKeyDown[index] && g_ctx.bPrevKeyDown[index];
}

bool IsMouseUp(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return !g_ctx.bKeyDown[index] && g_ctx.bPrevKeyDown[index];
}

bool IsMouseReleased(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return !g_ctx.bKeyDown[index] && g_ctx.bPrevKeyDown[index];
}

bool IsMouseAway(const eMouse _mouse) noexcept
{
    int index = EnumToInt(_mouse);
    return !g_ctx.bKeyDown[index] && !g_ctx.bPrevKeyDown[index];
}

std::pair<Int32, Int32> GetMousePos() noexcept
{
    return { g_ctx.mouseX, g_ctx.mouseY };
}

Int32 GetMouseDeltaWheel() noexcept
{
    return static_cast<Int32>(g_ctx.mouseDeltaWheel);
}

std::pair<Int32, Int32> GetMouseDeltaPos() noexcept
{
    const UInt32 deltaX = g_ctx.mouseX - g_ctx.mousePrevX;
    const UInt32 deltaY = g_ctx.mouseY - g_ctx.mousePrevY;
    return { deltaX, deltaY };
}

}   // namespace jam
