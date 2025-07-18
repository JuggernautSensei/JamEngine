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

    NODISCARD virtual std::string      ToString() const = 0;
    NODISCARD virtual std::string_view GetName() const  = 0;
    NODISCARD virtual UInt32           GetHash() const  = 0;
    NODISCARD bool                     IsHandled() const { return m_bHandled; }
    void                               SetHandled(const bool _bHandled = true) { m_bHandled = _bHandled; }

private:
    bool m_bHandled = false;
};

#define JAM_EVENT_IMPL(EventType, ...)                                                   \
    NODISCARD std::string ToString() const override { return std::format(__VA_ARGS__); } \
    NODISCARD std::string_view GetName() const override { return NameOf<EventType>(); }  \
    NODISCARD UInt32           GetHash() const override { return HashOf<EventType>(); }

#define JAM_EVENT(EventType, ...)                                         \
    JAM_EVENT_IMPL(EventType, __VA_ARGS__)                                \
    constexpr static std::string_view k_staticName = NameOf<EventType>(); \
    constexpr static UInt32           k_staticHash = HashOf<EventType>();

class EventDispatcher
{
public:
    template<typename EventType, typename ListenerType>
    void AddListener(ListenerType _listener)
    {
        using RawEventType = std::remove_cvref_t<EventType>;

        static_assert(std::is_base_of_v<Event, EventType>, "EventType must inherit from Event.");
        static_assert(std::is_invocable_v<ListenerType, RawEventType>, "ListenerType must be invocable with EventType as an argument.");

        JAM_ASSERT(!m_listeners.contains(EventType::k_staticHash), "Listener for event type '{}' already exists.", EventType::k_staticName);
        m_listeners.emplace(EventType::k_staticHash,
                            [_listener](Event& _eventRef) mutable
                            {
                                _listener(static_cast<RawEventType&>(_eventRef));
                            });
    }

    template<typename EventType, typename CallerType, typename ListenerType>
    void AddListener(CallerType* _pCaller, ListenerType _listener)
    {
        using RawEventType = std::remove_cvref_t<EventType>;

        static_assert(std::is_base_of_v<Event, EventType>, "EventType must inherit from Event.");
        static_assert(std::is_invocable_v<ListenerType, CallerType*, RawEventType>, "ListenerType must be invocable with EventType as an argument.");
        static_assert(std::is_member_function_pointer_v<ListenerType>, "ListenerType must be a member function pointer.");

        JAM_ASSERT(!m_listeners.contains(EventType::k_staticHash), "Listener for event type '{}' already exists.", EventType::k_staticName);
        m_listeners.emplace(EventType::k_staticHash,
                            [_pCaller, _listener](Event& _eventRef) mutable
                            {
                                (_pCaller->*_listener)(static_cast<RawEventType&>(_eventRef));
                            });
    }

    void Dispatch(Event& _eventRef) const
    {
        if (_eventRef.IsHandled())
        {
            return;
        }

        const UInt32 key = _eventRef.GetHash();
        const auto   it  = m_listeners.find(key);
        if (it != m_listeners.end())
        {
            it->second(_eventRef);
        }
    }

private:
    std::unordered_map<UInt32, std::function<void(Event&)>> m_listeners;
};

#define JAM_ADD_LISTENER_MEMBER_FUNCTION(Listener) this, &Listener

}   // namespace jam