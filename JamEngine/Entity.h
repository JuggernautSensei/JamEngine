#pragma once
#include "UID.h"
#include "UnremovableComponent.h"

// clang-format off
namespace jam { class TransformComponent; class TransformHierarchy; }
// clang-format on

namespace jam
{

class Entity
{
public:
    Entity() = default;
    Entity(entt::registry& _registry, entt::entity _handle, UID _uid);

    JAM_NODISCARD bool operator==(Entity _other) const;
    JAM_NODISCARD bool operator!=(Entity _other) const;

    JAM_NODISCARD entt::registry* GetRegistry() const;
    JAM_NODISCARD entt::entity GetHandle() const;
    JAM_NODISCARD bool         IsValid() const;

    JAM_NODISCARD UID                       GetUID() const;
    JAM_NODISCARD TransformComponent&       GetTransformComponent();
    JAM_NODISCARD const TransformComponent& GetTransformComponent() const;
    JAM_NODISCARD TransformHierarchy&       GetTransform();
    JAM_NODISCARD const TransformHierarchy& GetTransform() const;

    template<typename T, typename... Args>
    decltype(auto) CreateComponent(Args&&... args)
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->emplace_or_replace<T>(m_handle, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    decltype(auto) CreateOrReplaceComponent(Args&&... args)
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->emplace_or_replace<T>(m_handle, std::forward<Args>(args)...);
    }

    template<typename... T>
    decltype(auto) GetComponent() const
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->get<T...>(m_handle);
    }

    template<typename... T>
    decltype(auto) GetComponent()
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->get<T...>(m_handle);
    }

    template<typename... T>
    bool HasComponent() const
    {
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pRegistry->all_of<T...>(m_handle);
    }

    template<typename... T>
    void RemoveComponent()
    {
        static_assert((!std::is_base_of_v<UnremovableComponent<T>, T> && ...), "Attempting to remove an unremovable component");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        JAM_ASSERT(HasComponent<T...>(), "Entity does not have the specified component(s)");
        m_pRegistry->remove<T...>(m_handle);
    }

    static Entity s_null;

private:
    entt::registry* m_pRegistry = nullptr;
    entt::entity    m_handle    = entt::null;
    UID             m_uid       = k_invalidUID;
};

}   // namespace jam
