#pragma once
#include "Event.h"
#include "UID.h"

namespace jam
{

class WindowResizedEvent : public Event
{
public:
    WindowResizedEvent(const Int32 _width, const Int32 _height)
        : m_width(_width)
        , m_height(_height)
    {
    }

    [[nodiscard]] Int32 GetWidth() const
    {
        return m_width;
    }

    [[nodiscard]] Int32 GetHeight() const
    {
        return m_height;
    }

    [[nodiscard]] std::string ToString() const override
    {
        return std::format("WindowResizedEvent: width: {}, height: {}", m_width, m_height);
    }

    JAM_EVENT(WindowResizedEvent, eEventCategory::Window)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMaximizedEvent : public Event
{
public:
    WindowMaximizedEvent(const Int32 _width, const Int32 _height)
        : m_width(_width)
        , m_height(_height)
    {
    }

    [[nodiscard]] Int32 GetWidth() const
    {
        return m_width;
    }

    [[nodiscard]] Int32 GetHeight() const
    {
        return m_height;
    }

    [[nodiscard]] std::string ToString() const override
    {
        return std::format("WindowMaximizedEvent: width: {}, height: {}", m_width, m_height);
    }

    JAM_EVENT(WindowMaximizedEvent, eEventCategory::Window)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMinimizedEvent : public Event
{
public:
    WindowMinimizedEvent() = default;

    [[nodiscard]] std::string ToString() const override
    {
        return "WindowMinimizedEvent: WindowsPlatform is minimized";
    }

    JAM_EVENT(WindowMinimizedEvent, eEventCategory::Window)
};

class WindowMoveEvent : public Event
{
public:
    WindowMoveEvent(const Int32 _x, const Int32 _y)
        : m_x(_x)
        , m_y(_y)
    {
    }

    [[nodiscard]] Int32 GetX() const
    {
        return m_x;
    }

    [[nodiscard]] Int32 GetY() const
    {
        return m_y;
    }

    [[nodiscard]] std::string ToString() const override
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

    [[nodiscard]] std::string ToString() const override
    {
        return "WindowCloseEvent: WindowsPlatform is closing";
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

    [[nodiscard]] Int32 GetDelta() const
    {
        return m_delta;
    }

    [[nodiscard]] std::string ToString() const override
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

    [[nodiscard]] Int32 GetX() const
    {
        return m_x;
    }

    [[nodiscard]] Int32 GetY() const
    {
        return m_y;
    }

    [[nodiscard]] std::string ToString() const override
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

    [[nodiscard]] Int32 GetVKey() const
    {
        return m_vKey;
    }

    [[nodiscard]] std::string ToString() const override
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

    [[nodiscard]] Int32 GetVKey() const
    {
        return m_vKey;
    }

    [[nodiscard]] std::string ToString() const override
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

    [[nodiscard]] Int32 GetVKey() const
    {
        return m_key;
    }

    [[nodiscard]] std::string ToString() const override
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

    [[nodiscard]] Int32 GetVKey() const
    {
        return m_key;
    }

    [[nodiscard]] std::string ToString() const override
    {
        return std::format("KeyUpEvent: vKey: {}", m_key);
    }

    JAM_EVENT(KeyUpEvent, eEventCategory::Input)

private:
    Int32 m_key = 0;
};

class WindowFocusedEvent : public Event
{
public:
    WindowFocusedEvent() = default;
    [[nodiscard]] std::string ToString() const override
    {
        return "window focused";
    }
    JAM_EVENT(WindowFocusedEvent, eEventCategory::Window)
};

class WindowLostFocusEvent : public Event
{
public:
    WindowLostFocusEvent() = default;
    [[nodiscard]] std::string ToString() const override
    {
        return "window lost focus";
    }
    JAM_EVENT(WindowLostFocusEvent, eEventCategory::Window)
};

class SceneChangedEvent : public Event
{
public:
    [[nodiscard]] std::string ToString() const override
    {
        return "scene changed";
    }

    JAM_EVENT(SceneChangedEvent, eEventCategory::FileSystem)
};

class EntityCreatedEvent : public Event
{
public:
    explicit EntityCreatedEvent(const UID _uid)
        : m_uid(_uid)
    {
    }

    [[nodiscard]] UID GetEntityUID() const
    {
        return m_uid;
    }

    [[nodiscard]] std::string ToString() const override
    {
        return std::format("EntityCreateEvent: entity UID: {}", m_uid);
    }

    JAM_EVENT(EntityCreatedEvent, eEventCategory::SceneManager)

private:
    UID m_uid;
};

class EntityDestroyedEvent : public Event
{
public:
    explicit EntityDestroyedEvent(const UID _uid)
        : m_uid(_uid)
    {
    }
    [[nodiscard]] UID GetEntityUID() const
    {
        return m_uid;
    }
    [[nodiscard]] std::string ToString() const override
    {
        return std::format("EntityDestroyEvent: entity UID: {}", m_uid);
    }

    JAM_EVENT(EntityDestroyedEvent, eEventCategory::SceneManager)

private:
    UID m_uid;
};

class FileAddedEvent : public Event
{
public:
    explicit FileAddedEvent(const fs::path& _path)
        : m_path(_path)
    {
    }

    [[nodiscard]] fs::path GetPath() const
    {
        return m_path;
    }

    [[nodiscard]] std::string ToString() const override
    {
        return std::format("FileAddEvent: path: {}", m_path.string());
    }

    JAM_EVENT(FileAddedEvent, eEventCategory::FileSystem);

private:
    fs::path m_path;
};

class FileDeletedEvent : public Event
{
public:
    explicit FileDeletedEvent(const fs::path& _path)
        : m_path(_path)
    {
    }

    [[nodiscard]] fs::path GetPath() const
    {
        return m_path;
    }

    [[nodiscard]] std::string ToString() const override
    {
        return std::format("FileDeleteEvent: path: {}", m_path.string());
    }

    JAM_EVENT(FileDeletedEvent, eEventCategory::FileSystem);

private:
    fs::path m_path;
};

class FileModifiedEvent : public Event
{
public:
    explicit FileModifiedEvent(const fs::path& _path)
        : m_path(_path)
    {
    }

    [[nodiscard]] fs::path GetPath() const
    {
        return m_path;
    }

    [[nodiscard]] std::string ToString() const override
    {
        return std::format("FileModifiedEvent: path: {}", m_path.string());
    }

    JAM_EVENT(FileModifiedEvent, eEventCategory::FileSystem);

private:
    fs::path m_path;
};

class FileRenamedEvent : public Event
{
public:
    explicit FileRenamedEvent(const fs::path& _old, const fs::path& _new)
        : m_old(_old)
        , m_new(_new)
    {
    }

    [[nodiscard]] fs::path GetOldPath() const
    {
        return m_old;
    }

    [[nodiscard]] fs::path GetNewPath() const
    {
        return m_new;
    }

    [[nodiscard]] std::string ToString() const override
    {
        return std::format("FileRenamedEvent: old path: {}, new path: {}", m_old.string(), m_new.string());
    }

    JAM_EVENT(FileRenamedEvent, eEventCategory::FileSystem);

private:
    fs::path m_old;
    fs::path m_new;
};

}   // namespace jam