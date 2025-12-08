#pragma once
#include "Properties.h"
#include "UID.h"

namespace jam
{

class TransformHierarchy;

class Entity
{
public:
    Entity() = default;
    Entity(entt::registry& _registry, entt::entity _handle, UID _uid);

    [[nodiscard]] bool operator==(Entity _other) const;
    [[nodiscard]] bool operator!=(Entity _other) const;

    [[nodiscard]] entt::registry* GetRegistry() const;
    [[nodiscard]] entt::entity    GetHandle() const;

    [[nodiscard]] bool IsValid() const;
    explicit           operator bool() const;

    [[nodiscard]] UID                       GetUID() const;
    [[nodiscard]] TransformHierarchy&       GetTransformHierarchy();
    [[nodiscard]] const TransformHierarchy& GetTransformHierarchy() const;

    template<typename T, typename... Args>
    decltype(auto) CreateComponent(Args&&... args)
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
        static_assert((!std::is_base_of_v<CannotRemovableComponent<T>, T> && ...), "Attempting to remove an unremovable component");
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
