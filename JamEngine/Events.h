#pragma once
#include "Event.h"
#include "UID.h"

namespace jam
{

class WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(const Int32 _width, const Int32 _height)
        : m_width(_width)
        , m_height(_height)
    {
    }

    JAM_NODISCARD Int32 GetWidth() const
    {
        return m_width;
    }

    JAM_NODISCARD Int32 GetHeight() const
    {
        return m_height;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("WindowResizeEvent: width: {}, height: {}", m_width, m_height);
    }

    JAM_EVENT(WindowResizeEvent, eEventCategory::Window)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMaximizeEvent : public Event
{
public:
    WindowMaximizeEvent(const Int32 _width, const Int32 _height)
        : m_width(_width)
        , m_height(_height)
    {
    }

    JAM_NODISCARD Int32 GetWidth() const
    {
        return m_width;
    }

    JAM_NODISCARD Int32 GetHeight() const
    {
        return m_height;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("WindowMaximizeEvent: width: {}, height: {}", m_width, m_height);
    }

    JAM_EVENT(WindowMaximizeEvent, eEventCategory::Window)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMinimizeEvent : public Event
{
public:
    WindowMinimizeEvent() = default;

    JAM_NODISCARD std::string ToString() const override
    {
        return "WindowMinimizeEvent: Window is minimized";
    }

    JAM_EVENT(WindowMinimizeEvent, eEventCategory::Window)
};

class WindowMoveEvent : public Event
{
public:
    WindowMoveEvent(const Int32 _x, const Int32 _y)
        : m_x(_x)
        , m_y(_y)
    {
    }

    JAM_NODISCARD Int32 GetX() const
    {
        return m_x;
    }

    JAM_NODISCARD Int32 GetY() const
    {
        return m_y;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("WindowMoveEvent: x: {}, y: {}", m_x, m_y);
    }

    JAM_EVENT(WindowMoveEvent, eEventCategory::Window)

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() = default;

    JAM_NODISCARD std::string ToString() const override
    {
        return "WindowCloseEvent: Window is closing";
    }

    JAM_EVENT(WindowCloseEvent, eEventCategory::Window)
};

class MouseWheelEvent : public Event
{
public:
    explicit MouseWheelEvent(const Int32 _delta)
        : m_delta(_delta)
    {
    }

    JAM_NODISCARD Int32 GetDelta() const
    {
        return m_delta;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("MouseWheelEvent: delta: {}", m_delta);
    }

    JAM_EVENT(MouseWheelEvent, eEventCategory::Input)

private:
    Int32 m_delta = 0;
};

class MouseMoveEvent : public Event
{
public:
    MouseMoveEvent(const Int32 _x, const Int32 _y)
        : m_x(_x)
        , m_y(_y)
    {
    }

    JAM_NODISCARD Int32 GetX() const
    {
        return m_x;
    }

    JAM_NODISCARD Int32 GetY() const
    {
        return m_y;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("MouseMoveEvent: x: {}, y: {}", m_x, m_y);
    }

    JAM_EVENT(MouseMoveEvent, eEventCategory::Input)

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class MouseDownEvent : public Event
{
public:
    explicit MouseDownEvent(const Int32 _vKey)
        : m_vKey(_vKey)
    {
    }

    JAM_NODISCARD Int32 GetVKey() const
    {
        return m_vKey;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("MouseDownEvent: vKey: {}", m_vKey);
    }

    JAM_EVENT(MouseDownEvent, eEventCategory::Input)

private:
    Int32 m_vKey = 0;
};

class MouseUpEvent : public Event
{
public:
    explicit MouseUpEvent(const Int32 _vKey)
        : m_vKey(_vKey)
    {
    }

    JAM_NODISCARD Int32 GetVKey() const
    {
        return m_vKey;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("MouseUpEvent: vKey: {}", m_vKey);
    }

    JAM_EVENT(MouseUpEvent, eEventCategory::Input)

private:
    Int32 m_vKey = 0;
};

class KeyDownEvent : public Event
{
public:
    explicit KeyDownEvent(const Int32 _vKey)
        : m_key(_vKey)
    {
    }

    JAM_NODISCARD Int32 GetVKey() const
    {
        return m_key;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("KeyDownEvent: vKey: {}", m_key);
    }

    JAM_EVENT(KeyDownEvent, eEventCategory::Input)

private:
    Int32 m_key = 0;
};

class KeyUpEvent : public Event
{
public:
    explicit KeyUpEvent(const Int32 _vKey)
        : m_key(_vKey)
    {
    }

    JAM_NODISCARD Int32 GetVKey() const
    {
        return m_key;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("KeyUpEvent: vKey: {}", m_key);
    }

    JAM_EVENT(KeyUpEvent, eEventCategory::Input)

private:
    Int32 m_key = 0;
};

class SceneChangeEvent : public Event
{
public:
    explicit SceneChangeEvent(std::string _sceneName)
        : m_sceneName(std::move(_sceneName))
    {
    }

    JAM_NODISCARD std::string_view GetSceneName() const
    {
        return m_sceneName;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("SceneChangeEvent: scene name: {}", m_sceneName);
    }

    JAM_EVENT(SceneChangeEvent, eEventCategory::FileSystem)

private:
    std::string m_sceneName;
};

class EntityDestroyedEvent : public Event
{
public:
    explicit EntityDestroyedEvent(const UID _uid)
        : m_uid(_uid)
    {
    }

    JAM_NODISCARD UID GetUID() const
    {
        return m_uid;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("EntityDestroyedEvent: UID: {}", m_uid);
    }

    JAM_EVENT(EntityDestroyedEvent, eEventCategory::SceneManager)

private:
    UID m_uid = {};
};

class EntityCreatedEvent : public Event
{
public:
    explicit EntityCreatedEvent(const UID _uid)
        : m_uid(_uid)
    {
    }

    JAM_NODISCARD UID GetUID() const
    {
        return m_uid;
    }

    JAM_NODISCARD std::string ToString() const override
    {
        return std::format("EntityCreatedEvent: UID: {}", m_uid);
    }

    JAM_EVENT(EntityCreatedEvent, eEventCategory::SceneManager)

private:
    UID m_uid = {};
};

}   // namespace jam