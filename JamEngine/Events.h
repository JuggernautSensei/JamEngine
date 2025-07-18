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

}   // namespace jam