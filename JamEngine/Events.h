#pragma once
#include "Event.h"
#include "UID.h"

namespace jam
{

class WindowResizedEvent : public Event
{
public:
    WindowResizedEvent(Int32 _width, Int32 _height);

    [[nodiscard]] Int32       GetWidth() const;
    [[nodiscard]] Int32       GetHeight() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(WindowResizedEvent)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMaximizedEvent : public Event
{
public:
    WindowMaximizedEvent(Int32 _width, Int32 _height);

    [[nodiscard]] Int32       GetWidth() const;
    [[nodiscard]] Int32       GetHeight() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(WindowMaximizedEvent)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMinimizedEvent : public Event
{
public:
    WindowMinimizedEvent();

    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(WindowMinimizedEvent)
};

class WindowMoveEvent : public Event
{
public:
    WindowMoveEvent(Int32 _x, Int32 _y);

    [[nodiscard]] Int32       GetX() const;
    [[nodiscard]] Int32       GetY() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(WindowMoveEvent)

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent();

    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(WindowCloseEvent)
};

class MouseWheelEvent : public Event
{
public:
    explicit MouseWheelEvent(Int32 _delta);

    [[nodiscard]] Int32       GetDelta() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(MouseWheelEvent)

private:
    Int32 m_delta = 0;
};

class MouseMoveEvent : public Event
{
public:
    MouseMoveEvent(Int32 _x, Int32 _y);

    [[nodiscard]] Int32       GetX() const;
    [[nodiscard]] Int32       GetY() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(MouseMoveEvent)

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class MouseDownEvent : public Event
{
public:
    explicit MouseDownEvent(Int32 _vKey);

    [[nodiscard]] Int32       GetVKey() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(MouseDownEvent)

private:
    Int32 m_vKey = 0;
};

class MouseUpEvent : public Event
{
public:
    explicit MouseUpEvent(Int32 _vKey);

    [[nodiscard]] Int32       GetVKey() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(MouseUpEvent)

private:
    Int32 m_vKey = 0;
};

class KeyDownEvent : public Event
{
public:
    explicit KeyDownEvent(Int32 _vKey);

    [[nodiscard]] Int32       GetVKey() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(KeyDownEvent)

private:
    Int32 m_key = 0;
};

class KeyUpEvent : public Event
{
public:
    explicit KeyUpEvent(Int32 _vKey);

    [[nodiscard]] Int32       GetVKey() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(KeyUpEvent)

private:
    Int32 m_key = 0;
};

class WindowFocusedEvent : public Event
{
public:
    WindowFocusedEvent();

    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(WindowFocusedEvent)
};

class WindowLostFocusEvent : public Event
{
public:
    WindowLostFocusEvent();

    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(WindowLostFocusEvent)
};

class SceneChangedEvent : public Event
{
public:
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(SceneChangedEvent)
};

class EntityCreatedEvent : public Event
{
public:
    explicit EntityCreatedEvent(UID _uid);

    [[nodiscard]] UID         GetEntityUID() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(EntityCreatedEvent)

private:
    UID m_uid;
};

class EntityDestroyedEvent : public Event
{
public:
    explicit EntityDestroyedEvent(UID _uid);

    [[nodiscard]] UID         GetEntityUID() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(EntityDestroyedEvent)

private:
    UID m_uid;
};

class FileAddedEvent : public Event
{
public:
    explicit FileAddedEvent(const fs::path& _path);

    [[nodiscard]] fs::path    GetPath() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(FileAddedEvent);

private:
    fs::path m_path;
};

class FileDeletedEvent : public Event
{
public:
    explicit FileDeletedEvent(const fs::path& _path);

    [[nodiscard]] fs::path    GetPath() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(FileDeletedEvent);

private:
    fs::path m_path;
};

class FileModifiedEvent : public Event
{
public:
    explicit FileModifiedEvent(const fs::path& _path);

    [[nodiscard]] fs::path    GetPath() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(FileModifiedEvent);

private:
    fs::path m_path;
};

class FileRenamedEvent : public Event
{
public:
    explicit FileRenamedEvent(const fs::path& _old, const fs::path& _new);

    [[nodiscard]] fs::path    GetOldPath() const;
    [[nodiscard]] fs::path    GetNewPath() const;
    [[nodiscard]] std::string ToString() const override;

    JAM_EVENT(FileRenamedEvent);

private:
    fs::path m_old;
    fs::path m_new;
};

}   // namespace jam