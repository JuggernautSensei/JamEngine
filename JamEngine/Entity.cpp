#include "pch.h"

#include "Entity.h"

#include "Components.h"

namespace jam
{

Entity Entity::s_null = Entity {};

Entity::Entity(entt::registry& _registry, const entt::entity _handle, const UID _uid)
    : m_pRegistry(&_registry)
    , m_handle(_handle)
    , m_uid(_uid)
{
}

bool Entity::operator==(const Entity _other) const
{
    return m_handle == _other.m_handle && m_pRegistry == _other.m_pRegistry;
}

bool Entity::operator!=(const Entity _other) const
{
    return !(*this == _other);
}

entt::registry* Entity::GetRegistry() const
{
    return m_pRegistry;
}

entt::entity Entity::GetHandle() const
{
    return m_handle;
}

bool Entity::IsValid() const
{
    return m_pRegistry != nullptr
        && m_pRegistry->valid(m_handle);
}

UID Entity::GetUID() const
{
    return m_uid;
}

TransformComponent& Entity::GetTransformComponent()
{
    return m_pRegistry->get<TransformComponent>(m_handle);
}

const TransformComponent& Entity::GetTransformComponent() const
{
    return m_pRegistry->get<TransformComponent>(m_handle);
}

TransformHierarchy& Entity::GetTransform()
{
    return m_pRegistry->get<TransformComponent>(m_handle).transform;
}

const TransformHierarchy& Entity::GetTransform() const
{
    return m_pRegistry->get<TransformComponent>(m_handle).transform;
}

}   // namespace jam