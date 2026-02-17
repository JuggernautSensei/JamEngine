#pragma once

namespace Jam
{

class Event
{
public:
    Event()          = default;
    virtual ~Event() = default;

    Event(const Event&)                = default;
    Event& operator=(const Event&)     = default;
    Event(Event&&) noexcept            = default;
    Event& operator=(Event&&) noexcept = default;

    virtual void                     FormatEventMessage(ScratchBuffer& _buf) const = 0;   // for logging or debugging
    [[nodiscard]] virtual StringView GetTypeName() const                           = 0;
    [[nodiscard]] virtual UInt32     GetTypeHash() const                           = 0;

    [[nodiscard]] bool IsHandled() const;
    void               SetHandled(bool _bHandled);

private:
    bool m_bHandled = false;
};

#define JAM_EVENT(_eventT)                                                          \
public:                                                                             \
    constexpr static StringView k_typeName = NameOf<_eventT>();                     \
    constexpr static UInt32     k_typeHash = HashOf<_eventT>();                     \
    [[nodiscard]] StringView    GetTypeName() const override { return k_typeName; } \
    [[nodiscard]] UInt32        GetTypeHash() const override { return k_typeHash; } \
    constexpr static bool       __JAM_EVENT_MARKER__ = true

template<typename TEvent>
concept EventType = requires(TEvent _event) {
    { TEvent::__JAM_EVENT_MARKER__ };
    std::derived_from<TEvent, Event>;
};

template<typename TEvent, typename TFunc>
concept IsEventHandlerType =
    EventType<TEvent>
    && requires(TFunc _func, TEvent& _event) {{ _func(_event) }; };

template<typename TEvent, typename TCaller, typename TMethod>
concept IsEventHandlerMethodType =
    EventType<TEvent>
    && requires(TCaller* _caller, TMethod _method, TEvent& _event) {{ (_caller->*_method)(_event) }; };

class EventDispatcher
{
public:
    explicit EventDispatcher(Event& _event)
        : m_event(&_event)
        , m_typeHash(_event.GetTypeHash())
    {
    }

    template<EventType TEvent, typename TFunc>
        requires IsEventHandlerType<TEvent, TFunc>
    void Dispatch(TFunc&& _func)
    {
        if (m_event->IsHandled())
        {
            return;
        }

        if (m_typeHash == TEvent::k_typeHash)
        {
            TEvent& event = *static_cast<TEvent*>(m_event);
            _func(event);
        }
    }

    template<EventType TEvent, typename TCaller, typename TMethod>
        requires IsEventHandlerMethodType<TEvent, TCaller, TMethod>
    void Dispatch(TCaller* _caller, TMethod&& _method)
    {
        if (m_event->IsHandled())
        {
            return;
        }

        if (m_typeHash == TEvent::k_typeHash)
        {
            TEvent& event = *static_cast<TEvent*>(m_event);
            (_caller->*_method)(event);
        }
    }

private:
    Event* m_event    = nullptr;
    UInt32 m_typeHash = 0;
};

}   // namespace Jam
