#include "pch.h"

#include "Scene.h"

#include "Components.h"
#include "Entity.h"

namespace jam
{

Scene::Scene(const std::string_view _name)
    : m_name(_name)
{
}

Entity Scene::CreateEntity()
{
    Entity entity = { this, m_registry.create() };
    entity.CreateCompoenent<TransformComponent>();
    return entity;
}

Entity Scene::CreateEntity(UInt32 _hint)
{
    return CreateEntity(static_cast<entt::entity>(_hint));
}

Entity Scene::CreateEntity(const entt::entity _handle)
{
    if (m_registry.valid(_handle))
    {
        return { this, _handle };
    }
    else
    {
        Entity entity = { this, m_registry.create(_handle) };
        entity.CreateCompoenent<TransformComponent>();
        return entity;
    }
}

Entity Scene::CloneEntity(const Entity _entity)
{
    const Entity       e      = CreateEntity();
    const entt::entity handle = e.GetHandle();

    for (auto [id, set]: m_registry.storage())
    {
        if (!set.contains(handle))
        {
            const void* elem = set.value(_entity.GetHandle());
            set.push(handle, elem);
        }
    }

    return e;
}

void Scene::DestroyEntity(const Entity _entity)
{
    m_registry.destroy(_entity.GetHandle());
}

}   // namespace jam