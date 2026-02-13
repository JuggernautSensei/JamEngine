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
    constexpr static StringView k_typeName = NameOf<_eventT>();                     \
    constexpr static UInt32     k_typeHash = HashOf<_eventT>();                     \
    [[nodiscard]] StringView    GetTypeName() const override { return k_typeName; } \
    [[nodiscard]] UInt32        GetTypeHash() const override { return k_typeHash; } \
    constexpr static bool       k_bIsEvent = true;

template<typename T>
concept EventBasedType = requires(T _event) {
    { T::k_bIsEvent } -> std::convertible_to<bool>;
    std::is_base_of_v<Event, T>;
};

template<typename TEvent, typename TFunc>
concept EventHandlerFuncType = requires(
    TFunc   _func,
    TEvent& _event) {
    EventBasedType<TEvent>;
    { _func(_event) } -> std::same_as<void>;   // const or non-const member function or lambda
};

template<typename TEvent, typename TCaller, typename TMethod>
concept EventHandlerMethodType = requires(
    TCaller* _caller,
    TMethod  _method,
    TEvent&  _event) {
    EventBasedType<TEvent>;
    { (_caller->*_method)(_event) } -> std::same_as<void>;   // const or non-const member function
};

class EventDispatcher
{
public:
    explicit EventDispatcher(Event& _event)
        : m_event(&_event)
        , m_typeHash(_event.GetTypeHash())
    {
    }

    template<EventBasedType TEvent, typename TFunc>
        requires EventHandlerFuncType<TEvent, TFunc>
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

    template<EventBasedType TEvent, typename TCaller, typename TMethod>
        requires EventHandlerMethodType<TEvent, TCaller, TMethod>
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
