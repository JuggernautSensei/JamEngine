#include "pch.h"

#include "Scene.h"

#include "Application.h"
#include "Components.h"
#include "Entity.h"

namespace jam
{

Scene::Scene(const std::string_view _name)
    : m_name(_name)
{
}

void Scene::Clear()
{
    ClearEntities();
    m_assetManager.ClearAll();
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

Entity Scene::GetEntity(const entt::entity _handle) const
{
    Entity e = { const_cast<Scene*>(this), _handle };
    return e.IsValid() ? e : Entity::s_null;
}

Entity Scene::GetEntity(UInt32 _id) const
{
    return GetEntity(static_cast<entt::entity>(_id));
}

void Scene::DestroyEntity(const Entity _entity)
{
    // 바로 파괴하면 현 프레임에서 문제가 발생할 수 있기 때문에 지연처리
    GetApplication().SubmitCommand(
        [this, _entity]
        {
            m_registry.destroy(_entity.GetHandle());
        });
}

}   // namespace jam