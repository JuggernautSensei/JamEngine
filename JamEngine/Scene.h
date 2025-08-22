#pragma once
#include "AssetManager.h"

namespace jam
{

class Texture2D;
class Entity;
class Event;

class Scene
{
public:
    explicit Scene(std::string_view _name);
    virtual ~Scene() = default;

    Scene(const Scene&)                = delete;
    Scene& operator=(const Scene&)     = delete;
    Scene(Scene&&) noexcept            = default;
    Scene& operator=(Scene&&) noexcept = default;

    virtual void OnEnter();
    virtual void OnExit() {}

    virtual void OnUpdate(float _deltaTime) {}
    virtual void OnFinalUpdate(float _deltaTime) {}

    virtual void OnBeginRender() {}
    virtual void OnRender() {}
    virtual void OnRenderUI() {}
    virtual void OnEndRender() {}

    virtual void OnEvent(Event& _eventRef) {}

    NODISCARD std::string_view         GetName() const { return m_name; }   // scene displayName
    NODISCARD virtual const Texture2D& GetSceneTexture() const = 0;         // final rendering output (= final color buffer, final render target texture)

    // save load
    bool Save(const std::optional<fs::path>& _path = std::nullopt);   // 만약 _path가 null이면, 자동으로 경로가 지정됨. 대부분의 경우 경로를 명시할 필요가 없음
    bool Load(const std::optional<fs::path>& _path = std::nullopt);   // 만약 _path가 null이면, 자동으로 경로가 지정됨. 대부분의 경우 경로를 명시할 필요가 없음

    // serialization
    NODISCARD virtual Json OnSerialize() const { return Json::value_t::null; }
    virtual void           OnDeserialize(const Json& _json) {}

    // registry
    NODISCARD entt::registry& GetRegistry() { return m_registry; }
    NODISCARD const entt::registry& GetRegistry() const { return m_registry; }

    // clear
    void ClearEntities() { m_registry.clear(); }
    void Clear();

    // entt interface
    // CreateEntity() is your best friend
    // GetEntity(), CloneEntity(), DestroyEntity() use internal (this is not your best friend)
    // use class Entity interface for clone and destroy
    // if you want to find entity by id or key ... you should make your own map. (not supported)
    NODISCARD Entity CreateEntity();
    NODISCARD Entity CreateEntity(UInt32 _id);          // by id
    NODISCARD Entity GetEntity(entt::entity _handle);   // by handle
    NODISCARD Entity GetEntity(UInt32 _id);             // by id
    NODISCARD Entity CloneEntity(entt::entity _handle);
    void             DestroyEntity(entt::entity _handle);

    template<typename... Args>
    NODISCARD decltype(auto) CreateView()
    {
        return m_registry.view<Args...>();
    }

    template<typename... Args>
    NODISCARD decltype(auto) CreateView() const
    {
        return m_registry.view<Args...>();
    }

    // asset manager interface
    NODISCARD AssetManager&       GetAssetManagerRef() { return m_assetManager; }
    NODISCARD const AssetManager& GetAssetManager() const { return m_assetManager; }

protected:
    AssetManager   m_assetManager;
    std::string    m_name;
    entt::registry m_registry;
};

}   // namespace jam