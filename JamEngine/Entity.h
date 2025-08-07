#pragma once
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

    NODISCARD Scene* GetScene() const { return m_pScene; }
    NODISCARD entt::entity GetHandle() const { return m_entity; }
    NODISCARD UInt32       GetID() const { return entt::to_integral(m_entity); }

    template<typename Ty, typename... Args>
    decltype(auto) CreateCompoenent(Args&&... args)
    {
        static_assert(std::is_convertible_v<decltype(Ty::_jam_component_meta_registered_), bool>, "Component must be registered with JAM_COMPONENT macro");
        JAM_ASSERT(m_pScene, "Entity is not associated with a scene");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pScene->GetRegistry().emplace_or_replace<Ty>(m_entity, std::forward<Args>(args)...);
    }

    template<typename... Ty>
    decltype(auto) GetComponent() const
    {
        static_assert((std::is_convertible_v<decltype(Ty::_jam_component_meta_registered_), bool> && ...), "All components must be registered with JAM_COMPONENT macro");
        JAM_ASSERT(m_pScene, "Entity is not associated with a scene");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pScene->GetRegistry().get<Ty...>(m_entity);
    }

    template<typename... Ty>
    NODISCARD bool HasComponent() const
    {
        static_assert((std::is_convertible_v<decltype(Ty::_jam_component_meta_registered_), bool> && ...), "All components must be registered with JAM_COMPONENT macro");
        JAM_ASSERT(m_pScene, "Entity is not associated with a scene");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pScene->GetRegistry().all_of<Ty...>(m_entity);
    }

    template<typename... Ty>
    decltype(auto) RemoveComponent() const
    {
        static_assert((std::is_convertible_v<decltype(Ty::_jam_component_meta_registered_), bool> && ...), "All components must be registered with JAM_COMPONENT macro");
        JAM_ASSERT(m_pScene, "Entity is not associated with a scene");
        JAM_ASSERT(IsValid(), "Entity is not valid");
        return m_pScene->GetRegistry().remove<Ty...>(m_entity);
    }

    NODISCARD bool IsValid() const
    {
        return m_pScene != nullptr && m_pScene->GetRegistry().valid(m_entity);
    }

    static Entity s_null;

private:
    Scene*       m_pScene = nullptr;
    entt::entity m_entity = entt::null;
};

}   // namespace jam
