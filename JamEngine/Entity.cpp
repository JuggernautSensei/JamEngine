#include "pch.h"

#include "Entity.h"

#include "SceneComponent.h"

namespace Jam
{

Entity::Entity(
    Registry&            _registry,
    const RegistryHandle _handle,
    const UID            _uid)
    : m_pRegistry(&_registry)
    , m_handle(_handle)
    , m_uid(_uid)
{
}

bool Entity::operator==(
    const Entity _other) const
{
    return m_pRegistry == _other.m_pRegistry
        && m_handle == _other.m_handle;
}

bool Entity::operator!=(
    const Entity _other) const
{
    return !(*this == _other);
}

bool Entity::IsValid() const
{
    return m_pRegistry && m_pRegistry->valid(m_handle);
}

UID Entity::GetUID() const
{
    return m_uid;
}

RegistryHandle Entity::GetHandle() const
{
    return m_handle;
}

SceneNode& Entity::GetSceneNode()
{
    return m_pRegistry->get<SceneComponent>(m_handle).node;
}

const SceneNode& Entity::GetSceneNode() const
{
    return m_pRegistry->get<SceneComponent>(m_handle).node;
}

Entity::operator bool() const
{
    return IsValid();
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

const Entity Entity::k_null = Entity {};

}   // namespace Jam