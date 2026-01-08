#pragma once

namespace jam
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

    [[nodiscard]] virtual std::string ToString() const    = 0;
    [[nodiscard]] virtual StringView  GetTypeName() const = 0;
    [[nodiscard]] virtual UInt32      GetTypeHash() const = 0;

    [[nodiscard]] bool IsHandled() const;
    void               SetHandled(bool _bHandled);

private:
    bool m_bHandled = false;
};

#define JAM_EVENT(_eventT, _eventCategory)                                      \
    constexpr static StringView k_name = NameOf<_eventT>();                     \
    constexpr static UInt32     k_hash = HashOf<_eventT>();                     \
    [[nodiscard]] StringView    GetTypeName() const override { return k_name; } \
    [[nodiscard]] UInt32        GetTypeHash() const override { return k_hash; }

class EventDispatcher
{
public:
    template<typename EventType, typename ListenerType>
    void AddListener(ListenerType&& _listener)
    {
        static_assert(std::is_base_of_v<Event, EventType>, "EventType must inherit from Event.");
        static_assert(std::is_invocable_v<ListenerType, EventType>, "ListenerType must be invocable with EventType as an argument.");

        JAM_ASSERT(!m_listeners.contains(EventType::k_hash), "Listener for event type '{}' already exists.", EventType::k_name);
        m_listeners.emplace(EventType::k_hash,
                            [_listener](Event& _event)
                            {
                                _listener(static_cast<EventType&>(_event));
                            });
    }

    template<typename EventType, typename CallerType, typename ListenerType>
    void AddListener(CallerType* _pCaller, ListenerType&& _listener)
    {
        using RawEventType = std::remove_cvref_t<EventType>;

        static_assert(std::is_base_of_v<Event, EventType>, "EventType must inherit from Event.");
        static_assert(std::is_invocable_v<ListenerType, CallerType*, RawEventType>, "ListenerType must be invocable with EventType as an argument.");
        static_assert(std::is_member_function_pointer_v<ListenerType>, "ListenerType must be a member function pointer.");

        JAM_ASSERT(!m_listeners.contains(EventType::k_hash), "Listener for event type '{}' already exists.", EventType::k_name);
        m_listeners.emplace(EventType::k_hash,
                            [_pCaller, _listener](Event& _event)
                            {
                                (_pCaller->*_listener)(static_cast<RawEventType&>(_event));
                            });
    }

    void Dispatch(Event& _event) const
    {
        if (_event.IsHandled())
        {
            return;
        }

        const UInt32 hash = _event.GetTypeHash();
        const auto   it   = m_listeners.find(hash);
        if (it != m_listeners.end())
        {
            it->second(_event);
        }
    }

private:
    HashMap<UInt32, std::function<void(Event&)>> m_listeners;
};

template<typename EventType, typename ListenerType>
void HandleEvent(Event& _event, ListenerType&& _listenerType)
{
    if (_event.IsHandled())
    {
        return;
    }

    if (_event.GetTypeHash() == EventType::k_hash)
    {
        EventType& event = static_cast<EventType&>(_event);
        _listenerType(event);
    }
}

template<typename EventType, typename CallerType, typename ListenerType>
void HandleEvent(Event& _event, CallerType* _pCaller, ListenerType&& _listenerType)
{
    if (_event.IsHandled())
    {
        return;
    }

    if (_event.GetTypeHash() == EventType::k_hash)
    {
        EventType& event = static_cast<EventType&>(_event);
        (_pCaller->*_listenerType)(event);
    }
}

}   // namespace jam