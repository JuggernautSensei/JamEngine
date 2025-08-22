#pragma once

namespace jam
{

enum class eEventCategory
{
    None,
    Window,
    Input,
    Renderer,
    FileSystem,
    SceneLayer,
    AssetManager,
};

enum eEventCategoryFlags_ : UInt32
{
    eEventCategoryFlags_None         = EnumToInt(eEventCategory::None),
    eEventCategoryFlags_Window       = 1 << EnumToInt(eEventCategory::Window),
    eEventCategoryFlags_Input        = 1 << EnumToInt(eEventCategory::Input),
    eEventCategoryFlags_Renderer     = 1 << EnumToInt(eEventCategory::Renderer),
    eEventCategoryFlags_FileSystem   = 1 << EnumToInt(eEventCategory::FileSystem),
    eEventCategoryFlags_SceneLayer   = 1 << EnumToInt(eEventCategory::SceneLayer),
    eEventCategoryFlags_AssetManager = 1 << EnumToInt(eEventCategory::AssetManager),

    eEventCategoryFlags_All = (1 << EnumCount<eEventCategory>()) - 1   // All categories
};
using eEventCategoryFlags = std::underlying_type_t<eEventCategoryFlags_>;

class Event
{
public:
    Event()          = default;
    virtual ~Event() = default;

    Event(const Event&)                = default;
    Event& operator=(const Event&)     = default;
    Event(Event&&) noexcept            = default;
    Event& operator=(Event&&) noexcept = default;

    NODISCARD virtual std::string      ToString() const    = 0;
    NODISCARD virtual std::string_view GetName() const     = 0;
    NODISCARD virtual UInt32           GetHash() const     = 0;
    NODISCARD virtual eEventCategory   GetCategory() const = 0;

    // handling
    NODISCARD bool IsHandled() const { return m_bHandled; }
    void           SetHandled(const bool _bHandled = true) { m_bHandled = _bHandled; }

private:
    bool m_bHandled = false;
};

#define JAM_DECLARE_TOSTRING_FUNCTION(...)

#define JAM_EVENT(EventType, EventCategory, EventString)                    \
    /* static event information */                                          \
    constexpr static std::string_view s_name     = NameOf<EventType>();     \
    constexpr static UInt32           s_hash     = HashOf<EventType>();     \
    constexpr static eEventCategory   s_category = EventCategory;           \
                                                                            \
    NODISCARD std::string ToString() const override { return EventString; } \
    NODISCARD std::string_view GetName() const override { return s_name; }  \
    NODISCARD UInt32           GetHash() const override { return s_hash; }  \
    NODISCARD eEventCategory   GetCategory() const override { return s_category; }

/*  Example usage:
    JAM_EVENT(WindowResizeEvent,            <- event type
              eEventCategoryFlags_Window,   <- event category flags
              std::format("WindowResizeEvent: width: {}, height: {}", m_width, m_height)) <- event string
*/

class EventDispatcher
{
public:
    template<typename EventType, typename ListenerType>
    void AddListener(ListenerType _listener)
    {
        using RawEventType = std::remove_cvref_t<EventType>;

        static_assert(std::is_base_of_v<Event, EventType>, "EventType must inherit from Event.");
        static_assert(std::is_invocable_v<ListenerType, RawEventType>, "ListenerType must be invocable with EventType as an argument.");

        JAM_ASSERT(!m_listeners.contains(EventType::s_hash), "Listener for event type '{}' already exists.", EventType::s_name);
        m_listeners.emplace(EventType::s_hash,
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

        JAM_ASSERT(!m_listeners.contains(EventType::s_hash), "Listener for event type '{}' already exists.", EventType::s_name);
        m_listeners.emplace(EventType::s_hash,
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