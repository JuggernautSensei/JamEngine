#pragma once
#include "Entity.h"
#include "Event.h"

namespace Jam
{

class WindowResizedEvent : public Event
{
    JAM_EVENT_BODY(WindowResizedEvent);

public:
    WindowResizedEvent(Int32 _width, Int32 _height);
    [[nodiscard]] Int32  GetWidth() const;
    [[nodiscard]] Int32  GetHeight() const;
    [[nodiscard]] String ToString() const override;

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMaximizedEvent : public Event
{
    JAM_EVENT_BODY(WindowMaximizedEvent);

public:
    WindowMaximizedEvent(Int32 _width, Int32 _height);

    [[nodiscard]] Int32  GetWidth() const;
    [[nodiscard]] Int32  GetHeight() const;
    [[nodiscard]] String ToString() const override;

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMinimizedEvent : public Event
{
    JAM_EVENT_BODY(WindowMinimizedEvent);

public:
    WindowMinimizedEvent();
    [[nodiscard]] String ToString() const override;
};

class WindowMoveEvent : public Event
{
    JAM_EVENT_BODY(WindowMoveEvent);

public:
    WindowMoveEvent(Int32 _x, Int32 _y);

    [[nodiscard]] Int32  GetX() const;
    [[nodiscard]] Int32  GetY() const;
    [[nodiscard]] String ToString() const override;

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class WindowClosedEvent : public Event
{
    JAM_EVENT_BODY(WindowClosedEvent);

public:
    WindowClosedEvent();
    [[nodiscard]] String ToString() const override;
};

class MouseWheelEvent : public Event
{
    JAM_EVENT_BODY(MouseWheelEvent);

public:
    explicit MouseWheelEvent(Int32 _delta);
    [[nodiscard]] String ToString() const override;
    [[nodiscard]] Int32  GetDelta() const;

private:
    Int32 m_delta = 0;
};

class MouseMoveEvent : public Event
{
    JAM_EVENT_BODY(MouseMoveEvent);

public:
    MouseMoveEvent(Int32 _x, Int32 _y);

    [[nodiscard]] Int32  GetX() const;
    [[nodiscard]] Int32  GetY() const;
    [[nodiscard]] String ToString() const override;

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class MouseDownEvent : public Event
{
    JAM_EVENT_BODY(MouseDownEvent);

public:
    explicit MouseDownEvent(Int32 _vKey);

    [[nodiscard]] Int32  GetVKey() const;
    [[nodiscard]] String ToString() const override;

private:
    Int32 m_vKey = 0;
};

class MouseUpEvent : public Event
{
    JAM_EVENT_BODY(MouseUpEvent);

public:
    explicit MouseUpEvent(Int32 _vKey);

    [[nodiscard]] Int32  GetVKey() const;
    [[nodiscard]] String ToString() const override;

private:
    Int32 m_vKey = 0;
};

class KeyDownEvent : public Event
{
    JAM_EVENT_BODY(KeyDownEvent);

public:
    explicit KeyDownEvent(Int32 _vKey);

    [[nodiscard]] Int32  GetVKey() const;
    [[nodiscard]] String ToString() const override;

private:
    Int32 m_key = 0;
};

class KeyUpEvent : public Event
{
    JAM_EVENT_BODY(KeyUpEvent);

public:
    explicit KeyUpEvent(Int32 _vKey);

    [[nodiscard]] Int32  GetVKey() const;
    [[nodiscard]] String ToString() const override;

private:
    Int32 m_key = 0;
};

class WindowFocusedEvent : public Event
{
    JAM_EVENT_BODY(WindowFocusedEvent);

public:
    WindowFocusedEvent();
    [[nodiscard]] String ToString() const override;
};

class WindowLostFocusEvent : public Event
{
    JAM_EVENT_BODY(WindowLostFocusEvent);

public:
    WindowLostFocusEvent();
    [[nodiscard]] String ToString() const override;
};

class SceneChangedEvent : public Event
{
    JAM_EVENT_BODY(SceneChangedEvent);

public:
    [[nodiscard]] String ToString() const override;
};

class EntityCreatedEvent : public Event
{
    JAM_EVENT_BODY(EntityCreatedEvent);

public:
    explicit EntityCreatedEvent(Entity _entity);
    [[nodiscard]] Entity GetEntity() const;
    [[nodiscard]] String ToString() const override;

private:
    Entity m_entity = Entity::k_null;
};

class EntityDestroyedEvent : public Event
{
    JAM_EVENT_BODY(EntityDestroyedEvent);

public:
    explicit EntityDestroyedEvent(Entity _entity);
    [[nodiscard]] Entity GetEntity() const;
    [[nodiscard]] String ToString() const override;

private:
    Entity m_entity = Entity::k_null;
};

class FileAddedEvent : public Event
{
    JAM_EVENT_BODY(FileAddedEvent);

public:
    explicit FileAddedEvent(StringView _path);
    [[nodiscard]] String GetPath() const;
    [[nodiscard]] String ToString() const override;

private:
    String m_path = {};
};

class FileDeletedEvent : public Event
{
    JAM_EVENT_BODY(FileDeletedEvent);

public:
    explicit FileDeletedEvent(StringView _path);

    [[nodiscard]] String GetPath() const;
    [[nodiscard]] String ToString() const override;

private:
    String m_path = {};
};

class FileModifiedEvent : public Event
{
    JAM_EVENT_BODY(FileModifiedEvent);

public:
    explicit FileModifiedEvent(StringView _path);

    [[nodiscard]] String GetPath() const;
    [[nodiscard]] String ToString() const override;

private:
    String m_path = {};
};

class FileRenamedEvent : public Event
{
    JAM_EVENT_BODY(FileRenamedEvent);

public:
    explicit FileRenamedEvent(StringView _old, StringView _new);

    [[nodiscard]] String GetOldPath() const;
    [[nodiscard]] String GetNewPath() const;
    [[nodiscard]] String ToString() const override;

private:
    String m_old = {};
    String m_new = {};
};

}   // namespace Jam