#pragma once
#include "Event.h"
#include "UID.h"

namespace Jam
{

class WindowResizedEvent : public Event
{
public:
    WindowResizedEvent(Int32 _width, Int32 _height);

    [[nodiscard]] Int32 GetWidth() const;
    [[nodiscard]] Int32 GetHeight() const;
    void                FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(WindowResizedEvent)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMaximizedEvent : public Event
{
public:
    WindowMaximizedEvent(Int32 _width, Int32 _height);

    [[nodiscard]] Int32 GetWidth() const;
    [[nodiscard]] Int32 GetHeight() const;
    void                FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(WindowMaximizedEvent)

private:
    Int32 m_width  = 0;
    Int32 m_height = 0;
};

class WindowMinimizedEvent : public Event
{
public:
    WindowMinimizedEvent();
    void FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(WindowMinimizedEvent)
};

class WindowMoveEvent : public Event
{
public:
    WindowMoveEvent(Int32 _x, Int32 _y);

    [[nodiscard]] Int32 GetX() const;
    [[nodiscard]] Int32 GetY() const;
    void                FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(WindowMoveEvent)

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent();
    void FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(WindowCloseEvent)
};

class MouseWheelEvent : public Event
{
public:
    explicit MouseWheelEvent(Int32 _delta);
    void FormatEventMessage(ScratchBuffer& _buf) const override;

    [[nodiscard]] Int32 GetDelta() const;

    JAM_EVENT(MouseWheelEvent)

private:
    Int32 m_delta = 0;
};

class MouseMoveEvent : public Event
{
public:
    MouseMoveEvent(Int32 _x, Int32 _y);

    [[nodiscard]] Int32 GetX() const;
    [[nodiscard]] Int32 GetY() const;
    void                FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(MouseMoveEvent)

private:
    Int32 m_x = 0;
    Int32 m_y = 0;
};

class MouseDownEvent : public Event
{
public:
    explicit MouseDownEvent(Int32 _vKey);

    [[nodiscard]] Int32 GetVKey() const;
    void                FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(MouseDownEvent)

private:
    Int32 m_vKey = 0;
};

class MouseUpEvent : public Event
{
public:
    explicit MouseUpEvent(Int32 _vKey);

    [[nodiscard]] Int32 GetVKey() const;
    void                FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(MouseUpEvent)

private:
    Int32 m_vKey = 0;
};

class KeyDownEvent : public Event
{
public:
    explicit KeyDownEvent(Int32 _vKey);

    [[nodiscard]] Int32 GetVKey() const;
    void                FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(KeyDownEvent)

private:
    Int32 m_key = 0;
};

class KeyUpEvent : public Event
{
public:
    explicit KeyUpEvent(Int32 _vKey);

    [[nodiscard]] Int32 GetVKey() const;
    void                FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(KeyUpEvent)

private:
    Int32 m_key = 0;
};

class WindowFocusedEvent : public Event
{
public:
    WindowFocusedEvent();
    void FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(WindowFocusedEvent)
};

class WindowLostFocusEvent : public Event
{
public:
    WindowLostFocusEvent();
    void FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(WindowLostFocusEvent)
};

class SceneChangedEvent : public Event
{
public:
    void FormatEventMessage(ScratchBuffer& _buf) const override;
    JAM_EVENT(SceneChangedEvent)
};

class EntityCreatedEvent : public Event
{
public:
    explicit EntityCreatedEvent(UID _uid);

    [[nodiscard]] UID GetEntityUID() const;
    void              FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(EntityCreatedEvent)

private:
    UID m_uid = UID::k_invalid;
};

class EntityDestroyedEvent : public Event
{
public:
    explicit EntityDestroyedEvent(UID _uid);

    [[nodiscard]] UID GetEntityUID() const;
    void              FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(EntityDestroyedEvent)

private:
    UID m_uid = {};
};

class FileAddedEvent : public Event
{
public:
    explicit FileAddedEvent(const Path& _path);

    [[nodiscard]] Path GetPath() const;
    void               FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(FileAddedEvent);

private:
    Path m_path = {};
};

class FileDeletedEvent : public Event
{
public:
    explicit FileDeletedEvent(const Path& _path);

    [[nodiscard]] Path GetPath() const;
    void               FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(FileDeletedEvent);

private:
    Path m_path = {};
};

class FileModifiedEvent : public Event
{
public:
    explicit FileModifiedEvent(const Path& _path);

    [[nodiscard]] Path GetPath() const;
    void               FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(FileModifiedEvent);

private:
    Path m_path = {};
};

class FileRenamedEvent : public Event
{
public:
    explicit FileRenamedEvent(const Path& _old, const Path& _new);

    [[nodiscard]] Path GetOldPath() const;
    [[nodiscard]] Path GetNewPath() const;
    void               FormatEventMessage(ScratchBuffer& _buf) const override;

    JAM_EVENT(FileRenamedEvent);

private:
    Path m_old = {};
    Path m_new = {};
};

}   // namespace Jam