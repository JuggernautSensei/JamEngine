#include "pch.h"

#include "Scene.h"

#include "Application.h"
#include "Entity.h"
#include "EnvironmentComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "TransformHierarchy.h"

namespace jam
{

const entt::registry& Scene::GetRegistry() const
{
    return m_registry;
}

entt::registry& Scene::GetRegistry()
{
    return m_registry;
}

TextureHandle Scene::GetDepthTexture() const
{
    return k_invalidHandle;
}

Entity Scene::CreateEntity(const std::string_view _name)
{
    const UID uid = m_uidGenerator.Alloc();
    return CreateEntity_(uid, _name);
}

Entity Scene::CreateEntity(const UID _uid, const std::string_view _name)
{
    const UID uid = m_uidGenerator.Alloc(_uid);
    return CreateEntity_(uid, _name);
}

Entity Scene::GetEntity(UID _uid) const
{
    auto it = m_entities.find(_uid);
    JAM_ASSERT(it != m_entities.end(), "Entity with UID {} does not exist", _uid);
    return it->second;
}

Entity Scene::FindEntity(const UID _uid) const
{
    auto it = m_entities.find(_uid);
    if (it == m_entities.end())
    {
        return Entity::s_null;
    }
    else
    {
        return it->second;
    }
}

Entity Scene::CloneEntity(const Entity _targetEntity)
{
    return CloneEntity_Recursive_(_targetEntity);
}

void Scene::DestroyEntity(const Entity _entity)
{
    JAM_ASSERT(_entity, "Entity is not valid");
    JAM_ASSERT(_entity != m_root, "Cannot delete root entity");

    struct EntityDelector
    {
        void operator()() const
        {
            pScene->DestroyEntity_Recursive_(entity);
        }

        Scene* pScene;
        Entity entity;
    };

    Application& app = Application::GetInstance();
    app.SubmitCommand(EntityDelector { this, _entity });
}

bool Scene::IsEntityValid(const Entity _entity) const
{
    return m_registry.valid(_entity.GetHandle());
}

Entity Scene::GetRootEntity() const
{
    return m_root;
}

Int32 Scene::GetEntityCount() const
{
    return NarrowCast<Int32>(m_entities.size());
}

const HashMap<UID, Entity>& Scene::GetEntities() const
{
    return m_entities;
}

Scene::Scene()
{
    m_root = CreateRootEntity_();
}

void Scene::DestroyEntity_Recursive_(Entity _entity)
{
    JAM_ASSERT(_entity, "Entity is not valid");
    JAM_ASSERT(_entity != m_root, "Cannot delete root entity");

    // destroy all children recursively
    TransformHierarchy& th    = _entity.GetTransformHierarchy();
    Entity              child = th.GetFirstChild();
    while (child.IsValid())
    {
        Entity next = child.GetTransformHierarchy().GetNextSibling();
        DestroyEntity_Recursive_(child);
        child = next;
    }

    // orphanize destroyed entity
    th.MakeOrphan();

    // clean up resource
    UID uid = _entity.GetUID();
    m_entities.erase(uid);
    m_uidGenerator.Free(uid);
    m_registry.destroy(_entity.GetHandle());   // destroy

    // dispatch event
    EntityDestroyedEvent event { uid };
    Application::GetInstance().DispatchEvent(event);
}

Entity Scene::CloneEntity_Recursive_(Entity _srcEntity)
{
    JAM_ASSERT(_srcEntity, "Entity is not valid");
    JAM_ASSERT(_srcEntity != m_root, "Cannot clone root entity");

    // clone src entity
    Entity             entity = CreateEntity();
    const ComponentDB& db     = ComponentDB::GetInstance();
    for (UInt32 typeID: std::views::keys(m_registry.storage()))
    {
        // transformHierarchy component is handled separately
        if (typeID == HashOf<TransformComponent>())
        {
            TransformHierarchy&       th    = entity.GetTransformHierarchy();
            const TransformHierarchy& srcTh = _srcEntity.GetTransformHierarchy();
            th.SetWorldTransform(srcTh.GetWorldTransform());   // just copy world transform
        }
        else
        {
            if (db.Has(typeID, _srcEntity))
            {
                void* pSrcComp = db.Get(typeID, _srcEntity);
                db.CreateByCopy(typeID, entity, pSrcComp);
            }
        }
    }

    // clone children recursively
    TransformHierarchy& srcTh = _srcEntity.GetTransformHierarchy();
    Entity              child = srcTh.GetFirstChild();
    while (child)
    {
        Entity clonedChild = CloneEntity_Recursive_(child);
        clonedChild.GetTransformHierarchy().SetParent(entity, true);
        child = child.GetTransformHierarchy().GetNextSibling();
    }

    return entity;
}

Entity Scene::CreateEntity_(UID _uid, const std::string_view _name)
{
    JAM_ASSERT(m_uidGenerator.IsAllocated(_uid), "UID {} is not allocated", _uid);

    // create entt handle
    const entt::entity handle = m_registry.create();
    Entity             entity { m_registry, handle, _uid };

    // entity map
    JAM_ASSERT(!m_entities.contains(_uid), "Entity with same UID already exists");
    m_entities.emplace(_uid, entity);

    // add transformHierarchy component (essential component)
    entity.CreateComponent<TransformComponent>(TransformHierarchy { m_root, entity });

    // create tag component if need
    if (!_name.empty())
    {
        entity.CreateComponent<TagComponent>(_name);
    }

    // dispatch event
    EntityCreatedEvent event { _uid };
    Application::GetInstance().DispatchEvent(event);
    return entity;
}

Entity Scene::CreateRootEntity_()
{
    const UID uid = m_uidGenerator.Alloc(k_rootEntityReservedUID);

    // create entt handle
    const entt::entity handle = m_registry.create();
    Entity             root { m_registry, handle, uid };

    // entity map
    JAM_ASSERT(!m_entities.contains(uid), "Entity with same UID already exists");
    m_entities.emplace(uid, root);

    // add transformHierarchy component (essential component)
    root.CreateComponent<TransformComponent>(TransformHierarchy { root });

    // create tag component
    root.CreateComponent<TagComponent>("scene root");

    // dispatch event
    EntityCreatedEvent event { uid };
    Application::GetInstance().DispatchEvent(event);
    return root;
}

}   // namespace jam
