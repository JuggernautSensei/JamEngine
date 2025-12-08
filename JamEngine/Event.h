#pragma once

namespace jam
{

/// @brief 이벤트 카테고리 열거형
enum class eEventCategory : Int8
{
    None = 0,
    Window,
    Input,
    Graphics,
    FileSystem,
    SceneManager,
    AssetManager,
    Editor,
};

/// @brief 이벤트 카테고리 플래그 (비트마스크)
enum eEventCategoryFlags_ : UInt32
{
    eEventCategoryFlags_None         = EnumToInt(eEventCategory::None),
    eEventCategoryFlags_Window       = 1 << EnumToInt(eEventCategory::Window),
    eEventCategoryFlags_Input        = 1 << EnumToInt(eEventCategory::Input),
    eEventCategoryFlags_Graphics     = 1 << EnumToInt(eEventCategory::Graphics),
    eEventCategoryFlags_FileSystem   = 1 << EnumToInt(eEventCategory::FileSystem),
    eEventCategoryFlags_SceneManager = 1 << EnumToInt(eEventCategory::SceneManager),
    eEventCategoryFlags_AssetManager = 1 << EnumToInt(eEventCategory::AssetManager),

    eEventCategoryFlags_All = 0xFFFFFFFF
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

    [[nodiscard]] virtual std::string      ToString() const    = 0;
    [[nodiscard]] virtual std::string_view GetTypeName() const = 0;
    [[nodiscard]] virtual UInt32           GetTypeHash() const = 0;
    [[nodiscard]] virtual eEventCategory   GetCategory() const = 0;

    [[nodiscard]] bool IsHandled() const
    {
        return m_bHandled;
    }

    void SetHandled(const bool _bHandled)
    {
        m_bHandled = _bHandled;
    }

private:
    bool m_bHandled = false;   // Handled 이 true인 경우 이벤트가 더 이상 처리되지 않음
};

#define JAM_EVENT(_eventT, _eventCategory)                                         \
    constexpr static std::string_view k_name     = NameOf<_eventT>();              \
    constexpr static UInt32           k_hash     = HashOf<_eventT>();              \
    constexpr static eEventCategory   k_category = _eventCategory;                 \
                                                                                   \
    [[nodiscard]] std::string_view GetTypeName() const override { return k_name; } \
    [[nodiscard]] UInt32           GetTypeHash() const override { return k_hash; } \
    [[nodiscard]] eEventCategory   GetCategory() const override { return k_category; }

/// @brief 이벤트 디스패처
/// 이벤트를 받으면 등록된 리스너에게 전달하여 처리함
class EventDispatcher
{
public:
    /// @brief 리스너 추가 (람다 및 일반 함수용)
    /// @tparam EventType 이벤트 타입
    /// @tparam ListenerType 리스너 타입
    /// @param _listener 리스너 함수 (람다 또는 일반 함수)
    template<typename EventType, typename ListenerType>
    void AddListener(ListenerType&& _listener)
    {
        using RawEventType = std::remove_cvref_t<EventType>;

        static_assert(std::is_base_of_v<Event, EventType>, "EventType must inherit from Event.");
        static_assert(std::is_invocable_v<ListenerType, RawEventType>, "ListenerType must be invocable with EventType as an argument.");

        JAM_ASSERT(!m_listeners.contains(EventType::k_hash), "Listener for event type '{}' already exists.", EventType::k_name);
        m_listeners.emplace(EventType::k_hash,
                            [_listener](Event& _event) mutable
                            {
                                _listener(static_cast<RawEventType&>(_event));
                            });
    }

    /// @brief 멤버 함수 리스너 추가
    /// @tparam EventType 이벤트 타입
    /// @tparam CallerType 호출자 타입
    /// @tparam ListenerType 멤버 함수 포인터 타입
    /// @param _pCaller 호출자 객체 포인터
    /// @param _listener 멤버 함수 포인터
    template<typename EventType, typename CallerType, typename ListenerType>
    void AddListener(CallerType* _pCaller, ListenerType&& _listener)
    {
        using RawEventType = std::remove_cvref_t<EventType>;

        static_assert(std::is_base_of_v<Event, EventType>, "EventType must inherit from Event.");
        static_assert(std::is_invocable_v<ListenerType, CallerType*, RawEventType>, "ListenerType must be invocable with EventType as an argument.");
        static_assert(std::is_member_function_pointer_v<ListenerType>, "ListenerType must be a member function pointer.");

        JAM_ASSERT(!m_listeners.contains(EventType::k_hash), "Listener for event type '{}' already exists.", EventType::k_name);
        m_listeners.emplace(EventType::k_hash,
                            [_pCaller, _listener](Event& _event) mutable
                            {
                                (_pCaller->*_listener)(static_cast<RawEventType&>(_event));
                            });
    }

    /// @brief 이벤트 디스패치
    /// @param _event 이벤트 참조
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

/// @brief 수동 이벤트 핸들링 (람다 및 일반 함수용)
/// @tparam EventType 이벤트 타입
/// @tparam ListenerType 리스너 타입
/// @param _event 이벤트 참조
/// @param _listenerType 리스너 함수 (람다 또는 일반 함수)
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

/// @brief 수동 이벤트 핸들링 (멤버 함수용)
/// @tparam EventType 이벤트 타입
/// @tparam CallerType 호출자 타입
/// @tparam ListenerType 리스너 타입
/// @param _event 이벤트 참조
/// @param _pCaller 호출자 객체 포인터
/// @param _listenerType 리스너 멤버 함수 포인터
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

#define JAM_BIND_MEMBER_LISTENER(Listener) this, &Listener

}   // namespace jam