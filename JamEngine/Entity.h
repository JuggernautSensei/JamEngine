// ReSharper disable CppMemberFunctionMayBeConst
#pragma once
#include "Application.h"
#include "ComponentMetadata.h"
#include "UID.h"

namespace Jam
{

class Transform;
class SceneNode;

class Entity
{
public:
    constexpr Entity() = default;
    Entity(entt::registry& _reg, entt::entity _handle, UID _uid);

    [[nodiscard]] bool operator==(Entity _other) const;
    [[nodiscard]] bool operator!=(Entity _other) const;
    explicit           operator bool() const;

    [[nodiscard]] bool         IsValid() const;
    [[nodiscard]] UID          GetUID() const;
    [[nodiscard]] entt::entity GetHandle() const;

    /////////// SCENE COMPONENT SHORT CUT //////////

    [[nodiscard]] SceneNode&       GetSceneNode();
    [[nodiscard]] const SceneNode& GetSceneNode() const;

    [[nodiscard]] const Transform& GetLocal() const;
    void                           SetLocal(const Transform& _local);
    [[nodiscard]] Transform&       ModifyLocal();

    [[nodiscard]] const Transform& GetWorld() const;
    void                           SetWorld(const Transform& _world);
    [[nodiscard]] Transform&       ModifyWorld();

    ////////// COMPONENT MANAGEMENT //////////

    template<typename T, typename... Args>
        requires HasComponentEssentialMarker<T>
    decltype(auto) CreateComponent(Args&&... _args)
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->emplace_or_replace<T>(m_handle, std::forward<Args>(_args)...);
    }

    template<typename T, typename... Args>
        requires HasComponentEssentialMarker<T>
    void CreateComponentDeferred(Args&&... _args)
    {
        Application& app = GetApp();
        app.SubmitCommandToMainThread(Bind(
            [this, ... args = std::forward<Args>(_args)] {
                CreateComponent<T>(std::move(args)...);
            }));
    }

    template<typename... T>
        requires(HasComponentEssentialMarker<T> && ...)
    decltype(auto) GetComponents() const
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->get<T...>(m_handle);
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    template<typename... T>
        requires(HasComponentEssentialMarker<T> && ...)
    decltype(auto) GetComponents()
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->get<T...>(m_handle);
    }

    template<typename... T>
        requires(HasComponentEssentialMarker<T> && ...)
    bool HaveComponents() const
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->all_of<T...>(m_handle);
    }

    template<typename... T>
        requires((HasComponentEssentialMarker<T> && !HasComponentPropertyFlag<T, eComponentPropertyFlags::NonRemoveable>) && ...)
    void RemoveComponents()
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        JAM_ASSERT(HaveComponents<T...>(), "Entity does not have the specified component(s)");
        m_pRegistry->remove<T...>(m_handle);
    }

    template<typename... T>
        requires((HasComponentEssentialMarker<T> && !HasComponentPropertyFlag<T, eComponentPropertyFlags::NonRemoveable>) && ...)
    void RemoveComponentsDeferred()
    {
        Application& app = GetApp();
        app.SubmitCommandToMainThread(Bind<&Entity::RemoveComponents<T...>>(this));
    }

    static const Entity kNull;

private:
    entt::registry* m_pRegistry = nullptr;
    entt::entity    m_handle    = entt::null;
    UID             m_uid       = UID::kNull;
};

}   // namespace Jam
