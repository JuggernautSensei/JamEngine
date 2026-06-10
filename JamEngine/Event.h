#pragma once
#include "String.h"

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

    [[nodiscard]] virtual String     ToString() const    = 0;
    [[nodiscard]] virtual StringView GetTypeName() const = 0;
    [[nodiscard]] virtual UInt64     GetTypeHash() const = 0;

    [[nodiscard]] bool IsHandled() const;
    void               SetHandled(bool _bHandled);

private:
    bool m_bHandled = false;
};

#define JAM_EVENT_BODY(_eventT)                                                    \
public:                                                                            \
    constexpr static StringView kTypeName = NameOf<_eventT>();                     \
    constexpr static UInt64     kTypeHash = HashOf<_eventT>();                     \
    [[nodiscard]] StringView    GetTypeName() const override { return kTypeName; } \
    [[nodiscard]] UInt64        GetTypeHash() const override { return kTypeHash; } \
    constexpr static bool       __JAM_EVENT_MARKER__ = true;                       \
                                                                                   \
private:                                                                           \
    static_assert(true)   // for semicolon after the macro

template<typename TEvent>
concept IsEventType = requires(TEvent _event) {
    { TEvent::__JAM_EVENT_MARKER__ };
    std::derived_from<TEvent, Event>;
};

template<typename TEvent, typename TFunc>
concept IsEventHandlerType =
    IsEventType<TEvent>
    && requires(TFunc _func, TEvent& _event) {{ _func(_event) }; };

template<typename TEvent, typename TCaller, typename TMethod>
concept IsEventHandlerMethodType =
    IsEventType<TEvent>
    && requires(TCaller* _caller, TMethod _method, TEvent& _event) {{ (_caller->*_method)(_event) }; };

class EventDispatcher
{
public:
    explicit EventDispatcher(Event& _event)
        : m_event(&_event)
        , m_typeHash(_event.GetTypeHash())
    {
    }

    template<IsEventType TEvent, typename TFunc>
        requires IsEventHandlerType<TEvent, TFunc>
    void Dispatch(TFunc&& _func)
    {
        if (m_event->IsHandled())
        {
            return;
        }

        if (m_typeHash == TEvent::kTypeHash)
        {
            TEvent& event = *static_cast<TEvent*>(m_event);
            _func(event);
        }
    }

    template<IsEventType TEvent, typename TCaller, typename TMethod>
        requires IsEventHandlerMethodType<TEvent, TCaller, TMethod>
    void Dispatch(TCaller* _caller, TMethod&& _method)
    {
        if (m_event->IsHandled())
        {
            return;
        }

        if (m_typeHash == TEvent::kTypeHash)
        {
            TEvent& event = *static_cast<TEvent*>(m_event);
            (_caller->*_method)(event);
        }
    }

private:
    Event* m_event    = nullptr;
    UInt64 m_typeHash = 0;
};

}   // namespace Jam
