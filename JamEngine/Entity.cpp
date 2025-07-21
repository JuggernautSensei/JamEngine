#include "pch.h"

#include "Entity.h"

namespace jam
{

Entity Entity::s_null = Entity();

Entity::Entity(Scene* _pScene, const entt::entity _entity)
    : m_pScene(_pScene)
    , m_entity(_entity)
{
    JAM_ASSERT(m_pScene, "Scene pointer is null");
    JAM_ASSERT(m_entity != entt::null, "Entity is null");
}

}   // namespace jam