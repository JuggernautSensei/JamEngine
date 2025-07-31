#pragma once
#include "Event.h"

namespace jam
{

class WindowResizeEvent final : public Event
{
public:
    WindowResizeEvent(const Int32 _width, const Int32 _height)
        : m_width(_width)
        , m_height(_height)
    {
    }

    NODISCARD Int32 GetWidth() const { return m_width; }
    NODISCARD Int32 GetHeight() const { return m_height; }
    JAM_EVENT(WindowResizeEvent, "WindowResizeEvent: width: {}, height: {}", m_width, m_height)
private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMoveEvent final : public Event
{
public:
    WindowMoveEvent(const Int32 _x, const Int32 _y)
        : m_x(_x)
        , m_y(_y)
    {
    }

    NODISCARD Int32 GetX() const { return m_x; }
    NODISCARD Int32 GetY() const { return m_y; }
    JAM_EVENT(WindowMoveEvent, "WindowMoveEvent: x: {}, y: {}", m_x, m_y)
private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class WindowCloseEvent final : public Event
{
public:
    WindowCloseEvent() = default;
    JAM_EVENT(WindowCloseEvent, "WindowCloseEvent: Window is closing")
};

class MouseWheelEvent final : public Event
{
public:
    explicit MouseWheelEvent(const Int32 _delta)
        : m_delta(_delta)
    {
    }

    NODISCARD Int32 GetDelta() const { return m_delta; }
    JAM_EVENT(MouseWheelEvent, "MouseWheelEvent: delta: {}", m_delta)

private:
    Int32 m_delta = 0;
};

class MouseMoveEvent final : public Event
{
public:
    MouseMoveEvent(const UInt32 _x, const UInt32 _y)
        : m_x(_x)
        , m_y(_y)
    {
    }
    NODISCARD UInt32 GetX() const { return m_x; }
    NODISCARD UInt32 GetY() const { return m_y; }
    JAM_EVENT(MouseMoveEvent, "MouseMoveEvent: x: {}, y: {}", m_x, m_y)
private:
    UInt32 m_x = 0;
    UInt32 m_y = 0;
};

class MouseDownEvent final : public Event
{
public:
    explicit MouseDownEvent(const int _vKey)
        : m_vKey(_vKey)
    {
    }

    NODISCARD int GetVKey() const { return m_vKey; }
    JAM_EVENT(MouseDownEvent, "MouseDownEvent: vKey: {}", m_vKey)
private:
    int m_vKey = 0;
};

class MouseUpEvent final : public Event
{
public:
    explicit MouseUpEvent(const int _vKey)
        : m_vKey(_vKey)
    {
    }

    NODISCARD int GetVKey() const { return m_vKey; }
    JAM_EVENT(MouseUpEvent, "MouseUpEvent: vKey: {}", m_vKey)
private:
    int m_vKey = 0;
};

class KeyDownEvent final : public Event
{
public:
    explicit KeyDownEvent(const int _vKey)
        : m_key(_vKey)
    {
    }

    NODISCARD int GetVKey() const { return m_key; }
    JAM_EVENT(KeyDownEvent, "KeyDownEvent: vKey: {}", m_key)

private:
    int m_key = 0;
};

class KeyUpEvent final : public Event
{
public:
    explicit KeyUpEvent(const int _vKey)
        : m_key(_vKey)
    {
    }
    NODISCARD int GetVKey() const { return m_key; }
    JAM_EVENT(KeyUpEvent, "KeyUpEvent: vKey: {}", m_key)

private:
    int m_key = 0;
};

class BackBufferCleanupEvent final : public Event
{
public:
    BackBufferCleanupEvent() = default;
    JAM_EVENT(BackBufferCleanupEvent, "BackBufferCleanupEvent");
};

}   // namespace jam