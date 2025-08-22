#include "pch.h"

#include "Entity.h"

namespace jam
{

Entity Entity::s_null = Entity();

Entity::Entity(Scene* _pScene, const entt::entity _entity)
    : m_pScene(_pScene)
    , m_handle(_entity)
{
    JAM_ASSERT(m_pScene, "Scene pointer is null");
    JAM_ASSERT(m_handle != entt::null, "Entity is null");
}

bool Entity::operator==(const Entity& _other) const
{
    return m_pScene == _other.m_pScene && m_handle == _other.m_handle;
}

Entity Entity::Clone() const
{
    return m_pScene->CloneEntity(m_handle);
}

void Entity::Destroy() const
{
    JAM_ASSERT(m_pScene, "Entity is not associated with a scene");
    JAM_ASSERT(IsValid(), "Entity is not valid");
    m_pScene->DestroyEntity(m_handle);
}

}   // namespace jam