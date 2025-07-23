#pragma once

namespace jam
{
class Event;

class Entity;

class Scene
{
public:
    explicit Scene(std::string_view _name);
    virtual ~Scene() = default;

    Scene(const Scene&)                = delete;
    Scene& operator=(const Scene&)     = delete;
    Scene(Scene&&) noexcept            = default;
    Scene& operator=(Scene&&) noexcept = default;

    virtual void OnAttach() {}
    virtual void OnDetach() {}

    virtual void OnEnter() {}
    virtual void OnExit() {}

    virtual void OnUpdate(float _deltaTime) {}
    virtual void OnFinalUpdate(float _deltaTime) {}

    virtual void OnBeginRender() {}
    virtual void OnRender() {}
    virtual void OnRenderUI() {}
    virtual void OnEndRender() {}

    virtual void OnEvent(Event& _eventRef) {}
    NODISCARD std::string_view GetName() const { return m_name; }

    NODISCARD entt::registry& GetRegistry() { return m_registry; }
    NODISCARD const entt::registry& GetRegistry() const { return m_registry; }

    template<typename... Args>
    NODISCARD decltype(auto) CreateView()
    {
        return m_registry.view<Args...>();
    }

    NODISCARD Entity CreateEntity();
    NODISCARD Entity CreateEntity(UInt32 _hint);
    NODISCARD Entity CreateEntity(entt::entity _handle);
    NODISCARD Entity CloneEntity(Entity _entity);
    void             DestroyEntity(Entity _entity);

protected:
    std::string    m_name;
    entt::registry m_registry;
};

}   // namespace jam