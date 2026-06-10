#include "pch.h"

#include "Entity.h"

#include "Application.h"
#include "SceneComponent.h"

namespace Jam
{

Entity::Entity(
    entt::registry&    _reg,
    const entt::entity _handle,
    const UID          _uid)
    : m_pRegistry(&_reg)
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

Entity::operator bool() const
{
    return IsValid();
}

bool Entity::IsValid() const
{
    return m_pRegistry && m_pRegistry->valid(m_handle);
}

UID Entity::GetUID() const
{
    return m_uid;
}

entt::entity Entity::GetHandle() const
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

const Transform& Entity::GetLocal() const
{
    return GetSceneNode().GetLocal();
}

void Entity::SetLocal(const Transform& _local)
{
    GetSceneNode().SetLocal(_local);
}

Transform& Entity::ModifyLocal()
{
    return GetSceneNode().ModifyLocal();
}

const Transform& Entity::GetWorld() const
{
    return GetSceneNode().GetWorld();
}

void Entity::SetWorld(const Transform& _world)
{
    GetSceneNode().SetWorld(_world);
}

Transform& Entity::ModifyWorld()
{
    return GetSceneNode().ModifyWorld();
}

const Entity Entity::kNull = Entity {};

}   // namespace Jam