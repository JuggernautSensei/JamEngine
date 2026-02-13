#pragma once
#include "ComponentTag.h"
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
    Entity(SceneRegistry& _registry, EntityHandle _handle, UID _uid);

    [[nodiscard]] bool operator==(Entity _other) const;
    [[nodiscard]] bool operator!=(Entity _other) const;

    [[nodiscard]] bool             IsValid() const;
    [[nodiscard]] UID              GetUID() const;
    [[nodiscard]] EntityHandle     GetHandle() const;
    [[nodiscard]] SceneNode&       GetSceneNode();
    [[nodiscard]] const SceneNode& GetSceneNode() const;

    /* implicit */ operator bool() const;

    [[nodiscard]] const Transform& GetWorldTransform() const;
    [[nodiscard]] Transform&       ModifyWorldTransform();
    [[nodiscard]] const Transform& GetLocalTransform() const;
    [[nodiscard]] Transform&       ModifyLocalTransform();

    template<typename T, typename... Args>
        requires IsComponentReflected<T>
    decltype(auto) CreateComponent(Args&&... _args)
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->emplace_or_replace<T>(m_handle, std::forward<Args>(_args)...);
    }

    template<typename T, typename... Args>
        requires IsComponentReflected<T>
    void CreateComponentDefered(Args&&... _args)
    {
        JAM_APP.SubmitCommandToMainThread(Bind(
            [this, ... args = std::forward<Args>(_args)]
            {
                CreateComponent<T>(std::move(args)...);
            }));
    }

    template<typename... T>
        requires(IsComponentReflected<T>, ...)
    decltype(auto) GetComponent() const
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->get<T...>(m_handle);
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    template<typename... T>
        requires(IsComponentReflected<T>, ...)
    decltype(auto) GetComponent()
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->get<T...>(m_handle);
    }

    template<typename... T>
        requires(IsComponentReflected<T>, ...)
    bool HasComponent() const
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->all_of<T...>(m_handle);
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    template<typename... T>
        requires(IsComponentReflected<T>, ...)
    void RemoveComponent()
    {
        static_assert((!IsComponentTagEnabled<T, eComponentTag::NonRemoveable>, ...), "One or more components are marked as NonRemoveable and cannot be removed from the entity.");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        JAM_ASSERT(HasComponent<T...>(), "Entity does not have the specified component(s)");
        m_pRegistry->remove<T...>(m_handle);
    }

    template<typename... T>
        requires(IsComponentReflected<T>, ...)
    void RemoveComponentDefered()
    {
        JAM_APP.SubmitCommandToMainThread(Bind(
            [this]
            {
                RemoveComponent<T...>();
            }));
    }

    static Entity k_invalid;

private:
    SceneRegistry* m_pRegistry = nullptr;
    EntityHandle   m_handle    = entt::null;
    UID            m_uid       = UID::k_invalid;
};

}   // namespace Jam
