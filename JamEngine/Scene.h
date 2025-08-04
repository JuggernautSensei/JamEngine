#pragma once
#include "AssetManager.h"

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

    virtual void OnEnter() {}
    virtual void OnExit() {}

    virtual void OnUpdate(float _deltaTime) {}
    virtual void OnFinalUpdate(float _deltaTime) {}

    virtual void OnBeginRender() {}
    virtual void OnRender() {}
    virtual void OnRenderUI() {}
    virtual void OnEndRender() {}

    NODISCARD virtual Json OnSerialize() const { return Json::value_t::null; }
    NODISCARD virtual void OnDeserialize(const Json& _json) {}

    virtual void OnEvent(Event& _eventRef) {}
    NODISCARD std::string_view GetName() const { return m_name; }

    // registry
    NODISCARD entt::registry& GetRegistry() { return m_registry; }
    NODISCARD const entt::registry& GetRegistry() const { return m_registry; }

    // clear
    void ClearEntities() { m_registry.clear(); }
    void Clear();

    // entity
    NODISCARD Entity CreateEntity();
    NODISCARD Entity CreateEntity(UInt32 _hint);
    NODISCARD Entity CreateEntity(entt::entity _handle);
    NODISCARD Entity GetEntity(entt::entity _handle) const;
    NODISCARD Entity GetEntity(UInt32 _id) const;
    NODISCARD Entity CloneEntity(Entity _entity);
    void             DestroyEntity(Entity _entity);

    template<typename... Args>
    NODISCARD decltype(auto) CreateView()
    {
        return m_registry.view<Args...>();
    }

    // asset manager interface
    NODISCARD AssetManager&       GetAssetManager() { return m_assetManager; }
    NODISCARD const AssetManager& GetAssetManager() const { return m_assetManager; }

protected:
    AssetManager   m_assetManager;
    std::string    m_name;
    entt::registry m_registry;
};

}   // namespace jam