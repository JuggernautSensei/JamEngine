#include "pch.h"

#include "Input.h"

#include "Event.h"
#include "Events.h"

namespace
{

struct InputState
{
    jam::EventDispatcher eventDispatcher;

    bool bPrevKeyDown[256] = { false };
    bool bKeyDown[256]     = { false };

    jam::UInt32 mousePrevX      = 0;
    jam::UInt32 mousePrevY      = 0;
    jam::UInt32 mouseX          = 0;
    jam::UInt32 mouseY          = 0;
    jam::UInt32 mouseWheelDelta = 0;
};

InputState g_inputState;

}   // namespace

namespace jam
{

void Input::Initialize()
{
    EventDispatcher& dispatcher = g_inputState.eventDispatcher;
    dispatcher.AddListener<KeyDownEvent>(OnKeyDown_);
    dispatcher.AddListener<KeyUpEvent>(OnKeyUp_);
    dispatcher.AddListener<MouseDownEvent>(OnMouseDown_);
    dispatcher.AddListener<MouseUpEvent>(OnMouseUp_);
    dispatcher.AddListener<MouseMoveEvent>(OnMouseMove_);
    dispatcher.AddListener<MouseWheelEvent>(OnMouseWheel_);
}

void Input::Update()
{
    std::ranges::copy(g_inputState.bKeyDown, g_inputState.bPrevKeyDown);
    g_inputState.mousePrevX = g_inputState.mouseX;
    g_inputState.mousePrevY = g_inputState.mouseY;
    g_inputState.mouseWheelDelta = 0;
}

void Input::OnEvent(Event& _event)
{
    const EventDispatcher& dispatcher = g_inputState.eventDispatcher;
    dispatcher.Dispatch(_event);
}

void Input::OnMouseDown_(const MouseDownEvent& _event)
{
    const int vKey = _event.GetVKey();
    JAM_ASSERT(vKey >= 0 && vKey < 256, "Invalid virtual key code: {}", vKey);
    g_inputState.bKeyDown[vKey] = true;
}

void Input::OnMouseUp_(const MouseUpEvent& _event)
{
    const int vKey = _event.GetVKey();
    JAM_ASSERT(vKey >= 0 && vKey < 256, "Invalid virtual key code: {}", vKey);
    g_inputState.bKeyDown[vKey] = false;
}

void Input::OnMouseMove_(const MouseMoveEvent& _event)
{
    g_inputState.mouseX = _event.GetX();
    g_inputState.mouseY = _event.GetY();
}

void Input::OnMouseWheel_(const MouseWheelEvent& _event)
{
    g_inputState.mouseWheelDelta = _event.GetDelta();
}

void Input::OnKeyDown_(const KeyDownEvent& _event)
{
    const int vKey = _event.GetVKey();
    JAM_ASSERT(vKey >= 0 && vKey < 256, "Invalid virtual key code: {}", vKey);
    g_inputState.bKeyDown[vKey] = true;
}

void Input::OnKeyUp_(const KeyUpEvent& _event)
{
    const int vKey = _event.GetVKey();
    JAM_ASSERT(vKey >= 0 && vKey < 256, "Invalid virtual key code: {}", vKey);
    g_inputState.bKeyDown[vKey] = false;
}

}   // namespace jam
