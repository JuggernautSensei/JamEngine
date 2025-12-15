#include "pch.h"

#include "Events.h"

namespace jam
{

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

std::string WindowResizedEvent::ToString() const
{
    return std::format("WindowResizedEvent: width: {}, height: {}", m_width, m_height);
}

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

std::string WindowMaximizedEvent::ToString() const
{
    return std::format("WindowMaximizedEvent: width: {}, height: {}", m_width, m_height);
}

WindowMinimizedEvent::WindowMinimizedEvent() = default;

std::string WindowMinimizedEvent::ToString() const
{
    return "WindowMinimizedEvent: WindowsPlatform is minimized";
}

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

std::string WindowMoveEvent::ToString() const
{
    return std::format("WindowMoveEvent: x: {}, y: {}", m_x, m_y);
}

WindowCloseEvent::WindowCloseEvent() = default;

std::string WindowCloseEvent::ToString() const
{
    return "WindowCloseEvent: WindowsPlatform is closing";
}

MouseWheelEvent::MouseWheelEvent(const Int32 _delta)
    : m_delta(_delta)
{
}

Int32 MouseWheelEvent::GetDelta() const
{
    return m_delta;
}

std::string MouseWheelEvent::ToString() const
{
    return std::format("MouseWheelEvent: delta: {}", m_delta);
}

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

std::string MouseMoveEvent::ToString() const
{
    return std::format("MouseMoveEvent: x: {}, y: {}", m_x, m_y);
}

MouseDownEvent::MouseDownEvent(const Int32 _vKey)
    : m_vKey(_vKey)
{
}

Int32 MouseDownEvent::GetVKey() const
{
    return m_vKey;
}

std::string MouseDownEvent::ToString() const
{
    return std::format("MouseDownEvent: vKey: {}", m_vKey);
}

MouseUpEvent::MouseUpEvent(const Int32 _vKey)
    : m_vKey(_vKey)
{
}

Int32 MouseUpEvent::GetVKey() const
{
    return m_vKey;
}

std::string MouseUpEvent::ToString() const
{
    return std::format("MouseUpEvent: vKey: {}", m_vKey);
}

KeyDownEvent::KeyDownEvent(const Int32 _vKey)
    : m_key(_vKey)
{
}

Int32 KeyDownEvent::GetVKey() const
{
    return m_key;
}

std::string KeyDownEvent::ToString() const
{
    return std::format("KeyDownEvent: vKey: {}", m_key);
}

KeyUpEvent::KeyUpEvent(const Int32 _vKey)
    : m_key(_vKey)
{
}

Int32 KeyUpEvent::GetVKey() const
{
    return m_key;
}

std::string KeyUpEvent::ToString() const
{
    return std::format("KeyUpEvent: vKey: {}", m_key);
}

WindowFocusedEvent::WindowFocusedEvent() = default;

std::string WindowFocusedEvent::ToString() const
{
    return "window focused";
}

WindowLostFocusEvent::WindowLostFocusEvent() = default;

std::string WindowLostFocusEvent::ToString() const
{
    return "window lost focus";
}

std::string SceneChangedEvent::ToString() const
{
    return "scene changed";
}

EntityCreatedEvent::EntityCreatedEvent(const UID _uid)
    : m_uid(_uid)
{
}

UID EntityCreatedEvent::GetEntityUID() const
{
    return m_uid;
}

std::string EntityCreatedEvent::ToString() const
{
    return std::format("EntityCreateEvent: entity UID: {}", m_uid);
}

EntityDestroyedEvent::EntityDestroyedEvent(const UID _uid)
    : m_uid(_uid)
{
}

UID EntityDestroyedEvent::GetEntityUID() const
{
    return m_uid;
}

std::string EntityDestroyedEvent::ToString() const
{
    return std::format("EntityDestroyEvent: entity UID: {}", m_uid);
}

FileAddedEvent::FileAddedEvent(const fs::path& _path)
    : m_path(_path)
{
}

fs::path FileAddedEvent::GetPath() const
{
    return m_path;
}

std::string FileAddedEvent::ToString() const
{
    return std::format("FileAddEvent: path: {}", m_path.string());
}

FileDeletedEvent::FileDeletedEvent(const fs::path& _path)
    : m_path(_path)
{
}

fs::path FileDeletedEvent::GetPath() const
{
    return m_path;
}

std::string FileDeletedEvent::ToString() const
{
    return std::format("FileDeleteEvent: path: {}", m_path.string());
}

FileModifiedEvent::FileModifiedEvent(const fs::path& _path)
    : m_path(_path)
{
}

fs::path FileModifiedEvent::GetPath() const
{
    return m_path;
}

std::string FileModifiedEvent::ToString() const
{
    return std::format("FileModifiedEvent: path: {}", m_path.string());
}

FileRenamedEvent::FileRenamedEvent(const fs::path& _old, const fs::path& _new)
    : m_old(_old)
    , m_new(_new)
{
}

fs::path FileRenamedEvent::GetOldPath() const
{
    return m_old;
}

fs::path FileRenamedEvent::GetNewPath() const
{
    return m_new;
}

std::string FileRenamedEvent::ToString() const
{
    return std::format("FileRenamedEvent: old path: {}, new path: {}", m_old.string(), m_new.string());
}
}   // namespace jam