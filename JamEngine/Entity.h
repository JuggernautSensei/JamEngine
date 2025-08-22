#pragma once
#include "Application.h"
#include "Scene.h"

namespace jam
{

class Entity
{
public:
    Entity() = default;
    Entity(Scene* _pScene, entt::entity _entity);

    NODISCARD bool operator==(const Entity& _other) const;
    NODISCARD bool operator!=(const Entity& _other) const { return !(*this == _other); }

    NODISCARD entt::entity GetHandle() const { return m_handle; }
    NODISCARD Scene*       GetScene() const { return m_pScene; }
    NODISCARD UInt32       GetID() const { return entt::to_integral(m_handle); }

    NODISCARD Entity Clone() const;
    void             Destroy() const;

    template<typename Ty, typename... Args>
    decltype(auto) CreateComponent(Args&&... args)
    {
        static_assert(std::is_convertible_v<decltype(Ty::_jam_component_meta_registered_), bool>, "Component must be registered with JAM_COMPONENT macro");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pScene->GetRegistry().emplace_or_replace<Ty>(m_handle, std::forward<Args>(args)...);
    }

    template<typename... Ty>
    decltype(auto) GetComponent() const
    {
        static_assert((std::is_convertible_v<decltype(Ty::_jam_component_meta_registered_), bool> && ...), "All components must be registered with JAM_COMPONENT macro");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pScene->GetRegistry().get<Ty...>(m_handle);
    }

    template<typename... Ty>
    NODISCARD bool HasComponent() const
    {
        static_assert((std::is_convertible_v<decltype(Ty::_jam_component_meta_registered_), bool> && ...), "All components must be registered with JAM_COMPONENT macro");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pScene->GetRegistry().all_of<Ty...>(m_handle);
    }

    template<typename... Ty>
    void RemoveComponent() const
    {
        static_assert((std::is_convertible_v<decltype(Ty::_jam_component_meta_registered_), bool> && ...), "All components must be registered with JAM_COMPONENT macro");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        JAM_ASSERT(HasComponent<Ty...>(), "Entity does not have the specified component(s)");
        m_pScene->GetRegistry().remove<Ty...>(m_handle);
    }

    NODISCARD bool IsValid() const
    {
        return m_pScene != nullptr && m_pScene->GetRegistry().valid(m_handle);
    }

    static Entity s_null;

private:
    Scene*       m_pScene = nullptr;
    entt::entity m_handle = entt::null;
};

}   // namespace jam
