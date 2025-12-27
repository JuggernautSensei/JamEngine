#include "pch.h"

#include "Scene.h"

#include "Application.h"
#include "Entity.h"
#include "SceneComponent.h"
#include "SceneNode.h"
#include "TagComponent.h"

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
    return k_invalidHandle;   // override in derived class if needed
}

void Scene::UpdateSceneGraph_()
{
    JAM_ASSERT(m_root, "Root entity is null");
    SceneNode& node = m_root.GetSceneNode();
    node.UpdateDirty_(m_modifiedTransformEntities);   // collect modified nodes
}

Entity Scene::CreateEntity()
{
    const UID uid = m_uidGenerator.Alloc();
    return CreateEntity_(uid, k_nullopt);
}

Entity Scene::CreateEntity(
    const StringView _name)
{
    const UID uid = m_uidGenerator.Alloc();
    return CreateEntity_(uid, _name);
}

Entity Scene::CreateEntity(
    const UID        _uid,
    const StringView _name)
{
    const UID uid = m_uidGenerator.Alloc(_uid);
    return CreateEntity_(uid, _name);
}

Entity Scene::CreateEntity(
    const UID _uid)
{
    const UID uid = m_uidGenerator.Alloc(_uid);
    return CreateEntity_(uid, k_nullopt);
}

Entity Scene::GetEntity(
    UID _uid) const
{
    const auto it = m_entities.find(_uid);
    JAM_ASSERT(it != m_entities.end(), "Entity with UID {} does not exist", _uid);
    return it->second;
}

Entity Scene::FindEntity(
    const UID _uid) const
{
    const auto it = m_entities.find(_uid);
    if (it == m_entities.end())
    {
        return Entity::s_null;
    }
    else
    {
        return it->second;
    }
}

Entity Scene::CloneEntity(
    const Entity _entity)
{
    return CloneEntity_Recursive_(_entity);
}

void Scene::DestroyEntity(
    const Entity _entity)
{
    DestroyEntity_Recursive_(_entity);
}

bool Scene::IsEntityValid(
    const Entity _entity) const
{
    return m_registry.valid(_entity.GetHandle());
}

Entity Scene::GetRootEntity() const
{
    JAM_ASSERT(m_root, "Root entity is null");
    return m_root;
}

Int32 Scene::GetNumEntities() const
{
    return NarrowCast<Int32>(m_entities.size());
}

const HashMap<UID, Entity>& Scene::GetEntities() const
{
    return m_entities;
}

Vector<Entity> Scene::GetModifiedTransformEntities() const
{
    return m_modifiedTransformEntities;
}

Scene::Scene()
{
    m_root = CreateRootEntity_();
}

void Scene::DestroyEntity_Recursive_(Entity _entity)
{
    JAM_ASSERT(_entity, "Entity is not valid");
    JAM_ASSERT(_entity != m_root, "Cannot delete root entity");

    // bottom -> up

    // 자식들을 먼저 모두 파괴
    SceneNode& node  = _entity.GetSceneNode();
    Entity     child = node.GetFirstChild();
    while (child)
    {
        const Entity childSibling = child.GetSceneNode().GetNextSibling();
        DestroyEntity_Recursive_(child);
        child = childSibling;
    }

    // 씬 그래프에서 분리
    node.Orphanize();

    // 리소스 제거
    const UID uid = _entity.GetUID();
    m_entities.erase(uid);
    m_uidGenerator.Free(uid);
    m_registry.destroy(_entity.GetHandle());   // destroy

    // 이벤트 디스패치
    EntityDestroyedEvent event { uid };
    Application::GetInstance().DispatchEvent(event);
}

Entity Scene::CloneEntity_Recursive_(Entity _srcEntity)
{
    JAM_ASSERT(_srcEntity, "Entity is not valid");
    JAM_ASSERT(_srcEntity != m_root, "Cannot clone root entity");

    // top -> down

    // 새로운 엔티티 생성
    Entity entity = CreateEntity();

    // 컴포넌트 카피
    const ComponentDB& db = ComponentDB::GetInstance();
    for (const UInt32 typeId: m_registry.storage() | std::views::keys)
    {
        // scene component do not trivial copy
        // just copy transform
        if (typeId == HashOf<SceneComponent>())
        {
            entity.ModifyWorldTransform() = _srcEntity.GetWorldTransform();
        }
        else
        {
            if (db.Has(typeId, _srcEntity))
            {
                const void* pCompData = db.Get(typeId, _srcEntity);
                db.CreateByCopy(typeId, entity, pCompData);
            }
        }
    }

    // 자식들도 재귀적으로 클론
    const SceneNode& srcNode  = _srcEntity.GetSceneNode();
    Entity           srcChild = srcNode.GetFirstChild();
    while (srcChild)
    {
        Entity clonedChild = CloneEntity_Recursive_(srcChild);
        clonedChild.GetSceneNode().SetParent(entity, true);
        srcChild = srcChild.GetSceneNode().GetNextSibling();
    }

    // done
    return entity;
}

Entity Scene::CreateEntity_(
    UID                  _uid,
    Optional<StringView> _name)
{
    JAM_ASSERT(m_uidGenerator.IsAllocated(_uid), "UID {} is not allocated", _uid);

    // create entt handle
    const entt::entity handle = m_registry.create();
    Entity             entity = { m_registry, handle, _uid };

    // entity map
    JAM_ASSERT(!m_entities.contains(_uid), "Entity with same UID already exists");
    m_entities.emplace(_uid, entity);

    // add transformHierarchy component (essential component)
    entity.CreateComponent<SceneComponent>(SceneNode { m_root, entity });

    // create tag component if need
    if (_name)
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
    JAM_ASSERT(!m_root, "Root entity already exists");
    JAM_ASSERT(!m_uidGenerator.IsAllocated(k_reservedUidForRoot), "Root entity reserved UID is already allocated");

    // create entt handle
    const UID          uid    = m_uidGenerator.Alloc(k_reservedUidForRoot);
    const entt::entity handle = m_registry.create();
    Entity             root   = { m_registry, handle, uid };

    // entity map
    JAM_ASSERT(!m_entities.contains(uid), "Entity with same UID already exists");
    m_entities.emplace(uid, root);

    // add transformHierarchy component (essential component)
    root.CreateComponent<SceneComponent>(SceneNode { root });

    // create tag component
    root.CreateComponent<TagComponent>("scene root");

    // dispatch event
    EntityCreatedEvent event { uid };
    Application::GetInstance().DispatchEvent(event);
    return root;
}

}   // namespace jam
