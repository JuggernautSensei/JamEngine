#include "pch.h"

#include "Entity.h"

#include "SceneComponent.h"

namespace jam
{

Entity Entity::s_null = Entity {};

Entity::Entity()
{
}

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
    return m_pRegistry && m_pRegistry->valid(m_handle);
}

Entity::operator bool() const
{
    return IsValid();
}

UID Entity::GetUID() const
{
    return m_uid;
}

SceneNode& Entity::GetSceneNode()
{
    return m_pRegistry->get<SceneComponent>(m_handle).node;
}

const SceneNode& Entity::GetSceneNode() const
{
    return m_pRegistry->get<SceneComponent>(m_handle).node;
}

const Transform& Entity::GetWorldTransform() const
{
    return GetSceneNode().GetWorldTransform();
}

Transform& Entity::ModifyWorldTransform()
{
    return GetSceneNode().ModifyWorldTransform();
}

const Transform& Entity::GetLocalTransform() const
{
    return GetSceneNode().GetLocalTransform();
}

Transform& Entity::ModifyLocalTransform()
{
    return GetSceneNode().ModifyLocalTransform();
}

}   // namespace jam