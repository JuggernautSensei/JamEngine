#include "pch.h"

#include "Events.h"

namespace Jam
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

void WindowResizedEvent::FormatEventMessage(
    ScratchBuffer& _buf) const
{
    _buf.Format("Window Resized: width={}, height={}", m_width, m_height);
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

void WindowMaximizedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Window Maximized: width={}, height={}", m_width, m_height);
}

WindowMinimizedEvent::WindowMinimizedEvent() = default;

void WindowMinimizedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Window Minimized");
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

void WindowMoveEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Window Moved: x={}, y={}", m_x, m_y);
}

WindowCloseEvent::WindowCloseEvent() = default;

void WindowCloseEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Window Close Requested");
}

MouseWheelEvent::MouseWheelEvent(const Int32 _delta)
    : m_delta(_delta)
{
}

void MouseWheelEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Mouse Wheel: delta={}", m_delta);
}

Int32 MouseWheelEvent::GetDelta() const
{
    return m_delta;
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

void MouseMoveEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Mouse Move: x={}, y={}", m_x, m_y);
}

MouseDownEvent::MouseDownEvent(const Int32 _vKey)
    : m_vKey(_vKey)
{
}

Int32 MouseDownEvent::GetVKey() const
{
    return m_vKey;
}

void MouseDownEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Mouse Down: vKey={}", m_vKey);
}

MouseUpEvent::MouseUpEvent(const Int32 _vKey)
    : m_vKey(_vKey)
{
}

Int32 MouseUpEvent::GetVKey() const
{
    return m_vKey;
}

void MouseUpEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Mouse Up: vKey={}", m_vKey);
}

KeyDownEvent::KeyDownEvent(const Int32 _vKey)
    : m_key(_vKey)
{
}

Int32 KeyDownEvent::GetVKey() const
{
    return m_key;
}

void KeyDownEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Key Down: vKey={}", m_key);
}

KeyUpEvent::KeyUpEvent(const Int32 _vKey)
    : m_key(_vKey)
{
}

Int32 KeyUpEvent::GetVKey() const
{
    return m_key;
}

void KeyUpEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Key Up: vKey={}", m_key);
}

WindowFocusedEvent::WindowFocusedEvent() = default;

void WindowFocusedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Window Focused");
}

WindowLostFocusEvent::WindowLostFocusEvent() = default;

void WindowLostFocusEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Window Lost Focus");
}

void SceneChangedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Scene Changed");
}

EntityCreatedEvent::EntityCreatedEvent(const UID _uid)
    : m_uid(_uid)
{
}

UID EntityCreatedEvent::GetEntityUID() const
{
    return m_uid;
}

void EntityCreatedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Entity Created: UID={}", m_uid);
}

EntityDestroyedEvent::EntityDestroyedEvent(const UID _uid)
    : m_uid(_uid)
{
}

UID EntityDestroyedEvent::GetEntityUID() const
{
    return m_uid;
}

void EntityDestroyedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("Entity Destroyed: UID={}", m_uid);
}

FileAddedEvent::FileAddedEvent(const Path& _path)
    : m_path(_path)
{
}

Path FileAddedEvent::GetPath() const
{
    return m_path;
}

void FileAddedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("File Added: Path={}", m_path);
}

FileDeletedEvent::FileDeletedEvent(const Path& _path)
    : m_path(_path)
{
}

Path FileDeletedEvent::GetPath() const
{
    return m_path;
}

void FileDeletedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("File Deleted: Path={}", m_path);
}

FileModifiedEvent::FileModifiedEvent(const Path& _path)
    : m_path(_path)
{
}

Path FileModifiedEvent::GetPath() const
{
    return m_path;
}

void FileModifiedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("File Modified: Path={}", m_path);
}

FileRenamedEvent::FileRenamedEvent(const Path& _old, const Path& _new)
    : m_old(_old)
      , m_new(_new)
{
}

Path FileRenamedEvent::GetOldPath() const
{
    return m_old;
}

Path FileRenamedEvent::GetNewPath() const
{
    return m_new;
}

void FileRenamedEvent::FormatEventMessage(ScratchBuffer& _buf) const
{
    _buf.Format("File Renamed: OldPath={} NewPath={}", m_old, m_new);
}

}   // namespace Jam