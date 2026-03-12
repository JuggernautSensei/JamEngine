#include "pch.h"
#include "Events.h"

namespace Jam
{

// WindowResizedEvent
WindowResizedEvent::WindowResizedEvent(const Int32 _width, const Int32 _height)
    : m_width(_width)
    , m_height(_height)
{
}

Int32 WindowResizedEvent::GetWidth() const
{
    return m_width;
}

Int32 WindowResizedEvent::GetHeight() const
{
    return m_height;
}

String WindowResizedEvent::ToString() const
{
    return String::Formatted("WindowResizedEvent: {} x {}", m_width, m_height);
}

// WindowMaximizedEvent
WindowMaximizedEvent::WindowMaximizedEvent(const Int32 _width, const Int32 _height)
    : m_width(_width)
    , m_height(_height)
{
}

Int32 WindowMaximizedEvent::GetWidth() const
{
    return m_width;
}

Int32 WindowMaximizedEvent::GetHeight() const
{
    return m_height;
}

String WindowMaximizedEvent::ToString() const
{
    return String::Formatted("WindowMaximizedEvent: {} x {}", m_width, m_height);
}

// WindowMinimizedEvent
WindowMinimizedEvent::WindowMinimizedEvent()
{
}

String WindowMinimizedEvent::ToString() const
{
    return "WindowMinimizedEvent";
}

// WindowMoveEvent
WindowMoveEvent::WindowMoveEvent(const Int32 _x, const Int32 _y)
    : m_x(_x)
    , m_y(_y)
{
}

Int32 WindowMoveEvent::GetX() const
{
    return m_x;
}

Int32 WindowMoveEvent::GetY() const
{
    return m_y;
}

String WindowMoveEvent::ToString() const
{
    return String::Formatted("WindowMoveEvent: {}, {}", m_x, m_y);
}

WindowClosedEvent::WindowClosedEvent()
{
}

String WindowClosedEvent::ToString() const
{
    return "WindowClosedEvent";
}

// MouseWheelEvent
MouseWheelEvent::MouseWheelEvent(const Int32 _delta)
    : m_delta(_delta)
{
}

Int32 MouseWheelEvent::GetDelta() const
{
    return m_delta;
}

String MouseWheelEvent::ToString() const
{
    return String::Formatted("MouseWheelEvent: {}", m_delta);
}

// MouseMoveEvent
MouseMoveEvent::MouseMoveEvent(const Int32 _x, const Int32 _y)
    : m_x(_x)
    , m_y(_y)
{
}

Int32 MouseMoveEvent::GetX() const
{
    return m_x;
}

Int32 MouseMoveEvent::GetY() const
{
    return m_y;
}

String MouseMoveEvent::ToString() const
{
    return String::Formatted("MouseMoveEvent: {}, {}", m_x, m_y);
}

// MouseDownEvent
MouseDownEvent::MouseDownEvent(const Int32 _vKey)
    : m_vKey(_vKey)
{
}

Int32 MouseDownEvent::GetVKey() const
{
    return m_vKey;
}

String MouseDownEvent::ToString() const
{
    return String::Formatted("MouseDownEvent: {}", m_vKey);
}

// MouseUpEvent
MouseUpEvent::MouseUpEvent(const Int32 _vKey)
    : m_vKey(_vKey)
{
}

Int32 MouseUpEvent::GetVKey() const
{
    return m_vKey;
}

String MouseUpEvent::ToString() const
{
    return String::Formatted("MouseUpEvent: {}", m_vKey);
}

// KeyDownEvent
KeyDownEvent::KeyDownEvent(const Int32 _vKey)
    : m_key(_vKey)
{
}

Int32 KeyDownEvent::GetVKey() const
{
    return m_key;
}

String KeyDownEvent::ToString() const
{
    return String::Formatted("KeyDownEvent: {}", m_key);
}

// KeyUpEvent
KeyUpEvent::KeyUpEvent(const Int32 _vKey)
    : m_key(_vKey)
{
}

Int32 KeyUpEvent::GetVKey() const
{
    return m_key;
}

String KeyUpEvent::ToString() const
{
    return String::Formatted("KeyUpEvent: {}", m_key);
}

// WindowFocusedEvent
WindowFocusedEvent::WindowFocusedEvent()
{
}

String WindowFocusedEvent::ToString() const
{
    return "WindowFocusedEvent";
}

// WindowLostFocusEvent
WindowLostFocusEvent::WindowLostFocusEvent()
{
}

String WindowLostFocusEvent::ToString() const
{
    return "WindowLostFocusEvent";
}

// SceneChangedEvent
String SceneChangedEvent::ToString() const
{
    return "SceneChangedEvent";
}

// EntityCreatedEvent
EntityCreatedEvent::EntityCreatedEvent(const Entity _entity)
    : m_entity(_entity)
{}

Entity EntityCreatedEvent::GetEntity() const
{
    return m_entity;
}

String EntityCreatedEvent::ToString() const
{
    return String::Formatted("EntityCreatedEvent: {}", m_entity.GetUID());
}

// EntityDestroyedEvent
EntityDestroyedEvent::EntityDestroyedEvent(
    const Entity _entity)
    : m_entity(_entity)
{
}

Entity EntityDestroyedEvent::GetEntity() const
{
    return m_entity;
}

String EntityDestroyedEvent::ToString() const
{
    return String::Formatted("EntityDestroyedEvent: {}", m_entity.GetUID());
}

// FileAddedEvent
FileAddedEvent::FileAddedEvent(const StringView _path)
    : m_path(_path)
{
}

String FileAddedEvent::GetPath() const
{
    return m_path;
}

String FileAddedEvent::ToString() const
{
    return String::Formatted("FileAddedEvent: {}", m_path);
}

// FileDeletedEvent
FileDeletedEvent::FileDeletedEvent(const StringView _path)
    : m_path(_path)
{
}

String FileDeletedEvent::GetPath() const
{
    return m_path;
}

String FileDeletedEvent::ToString() const
{
    return String::Formatted("FileDeletedEvent: {}", m_path);
}

// FileModifiedEvent
FileModifiedEvent::FileModifiedEvent(const StringView _path)
    : m_path(_path)
{
}

String FileModifiedEvent::GetPath() const
{
    return m_path;
}

String FileModifiedEvent::ToString() const
{
    return String::Formatted("FileModifiedEvent: {}", m_path);
}

// FileRenamedEvent
FileRenamedEvent::FileRenamedEvent(const StringView _old, const StringView _new)
    : m_old(_old)
    , m_new(_new)
{
}

String FileRenamedEvent::GetOldPath() const
{
    return m_old;
}

String FileRenamedEvent::GetNewPath() const
{
    return m_new;
}

String FileRenamedEvent::ToString() const
{
    return String::Formatted("FileRenamedEvent: {} -> {}", m_old, m_new);
}

}   // namespace Jam