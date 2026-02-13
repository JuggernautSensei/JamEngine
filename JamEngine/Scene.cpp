#include "pch.h"

#include "Scene.h"

#include "Application.h"
#include "Entity.h"
#include "Events.h"
#include "Macros.h"
#include "Model.h"
#include "RenderableComponent.h"
#include "SceneComponent.h"
#include "SceneNode.h"
#include "TagComponent.h"
#include "Mesh.h"
#include "Material.h"
#include "ReflectionDB.h"

namespace Jam
{

TextureHandle Scene::GetDepthTexture() const
{
    return k_invalidHandle;   // override in derived class if needed
}

Entity Scene::CreateEntity(
    const StringView _nameOrEmpty)
{
    const UID uid = m_uidGenerator.Alloc();
    return CreateEntity_(uid, _nameOrEmpty);
}

Entity Scene::CreateEntity(
    const UID        _uid,
    const StringView _nameOrEmpty)
{
    const UID uid = m_uidGenerator.Alloc(_uid);
    return CreateEntity_(uid, _nameOrEmpty);
}

UID Scene::CreateEntityDefered(
    const StringView _nameOrEmpty)
{
    const UID uid = m_uidGenerator.Alloc();
    CreateEntityDefered_(uid, _nameOrEmpty);
    return uid;
}

UID Scene::CreateEntityDefered(
    const UID        _uid,
    const StringView _nameOrEmpty)
{
    const UID uid = m_uidGenerator.Alloc(_uid);
    CreateEntityDefered_(uid, _nameOrEmpty);
    return uid;
}

Entity Scene::CloneEntity(
    const Entity _entity)
{
    const UID uid = m_uidGenerator.Alloc();
    return CloneEntity_(uid, _entity);
}

UID Scene::CloneEntityDefered(
    const Entity _entity)
{
    const UID uid = m_uidGenerator.Alloc();
    CloneEntityDefered_(uid, _entity);
    return uid;
}

void Scene::DestroyEntity(
    const Entity _entity)
{
    DestroyEntity_Recursive_(_entity);
}

void Scene::DestroyEntityDefered(
    const Entity _entity)
{
    auto destroyEntity = [this, _entity]
    {
        DestroyEntity_Recursive_(_entity);
    };

    JAM_APP.SubmitCommandToMainThread(Bind(std::move(destroyEntity)));
}

Entity Scene::GetEntity(
    UID _uid) const
{
    JAM_ASSERT(_uid, "UID is invalid");
    const auto it = m_entities.find(_uid);
    JAM_ASSERT(it != m_entities.end(), "Entity with UID {} does not exist", _uid);
    return it->second;
}

Optional<Entity> Scene::FindEntity(
    const UID _uid) const
{
    if (!_uid.IsValid())
    {
        return k_nullopt;
    }

    const auto it = m_entities.find(_uid);
    if (it == m_entities.end())
    {
        return k_nullopt;
    }

    return it->second;
}

bool Scene::IsEntityValid(
    const Entity _entity) const
{
    return m_registry.valid(_entity.GetHandle());
}

Entity Scene::CreateEntitiesFrom(
    const Ref<Model>& _pModel)
{
    JAM_ASSERT(_pModel, "Model is null");
    const UID uid = m_uidGenerator.Alloc();
    return CreateEntitiesFrom_(uid, _pModel);
}

UID Scene::CreateEntitiesDeferedFrom(
    const Ref<Model>& _pModel)
{
    JAM_ASSERT(_pModel, "Model is null");
    const UID uid = m_uidGenerator.Alloc();
    CreateEntitiesFromDefered_(uid, _pModel);
    return uid;
}

const SceneRegistry& Scene::GetRegistry() const
{
    return m_registry;
}

SceneRegistry& Scene::GetRegistry()
{
    return m_registry;
}

Entity Scene::GetRootEntity() const
{
    JAM_ASSERT(m_root, "Root entity is null");
    return m_root;
}

Int32 Scene::GetNumEntities() const
{
    return NumericCast<Int32>(m_entities.size());
}

const HashMap<UID, Entity>& Scene::GetEntities() const
{
    return m_entities;
}

Vector<Entity> Scene::GetModifiedTransformEntities() const
{
    return m_transformModifiedEntities;
}

Scene::Scene()
{
    m_root = CreateRootEntity_();
}

Entity Scene::CreateRootEntity_()
{
    JAM_ASSERT(!m_root, "Root entity already exists");
    JAM_ASSERT(!m_uidGenerator.IsAllocated(k_rootEntityUid), "Root entity reserved UID is already allocated");

    // create entt handle
    const UID          uid    = m_uidGenerator.Alloc(k_rootEntityUid);
    const EntityHandle handle = m_registry.create();
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
    JAM_APP.DispatchEvent(event);
    return root;
}

Entity Scene::CreateEntity_(
    UID        _allocedUid,
    StringView _nameOrEmpty)
{
    JAM_ASSERT(m_uidGenerator.IsAllocated(_allocedUid), "UID {} is not allocated", _allocedUid);

    // create entt handle
    const EntityHandle handle = m_registry.create();
    Entity             entity = { m_registry, handle, _allocedUid };

    // entity map
    JAM_ASSERT(!m_entities.contains(_allocedUid), "Entity with same UID already exists");
    m_entities.emplace(_allocedUid, entity);

    // add transformHierarchy component (essential component)
    entity.CreateComponent<SceneComponent>(SceneNode { m_root, entity });

    // create tag component if need
    if (!_nameOrEmpty.IsEmpty())
    {
        entity.CreateComponent<TagComponent>(_nameOrEmpty);
    }

    // dispatch event
    EntityCreatedEvent event { _allocedUid };
    JAM_APP.DispatchEvent(event);
    return entity;
}

void Scene::CreateEntityDefered_(
    const UID        _allocedUid,
    const StringView _nameOrEmpty)
{
    const String name         = _nameOrEmpty.ToString();
    auto         createEntity = [this, _allocedUid, name]
    {
        CreateEntity_(_allocedUid, name);
    };

    JAM_APP.SubmitCommandToMainThread(Bind(std::move(createEntity)));
}

Entity Scene::CloneEntity_(
    const UID    _allocedUid,
    const Entity _srcEntity)
{
    // clone
    Entity entity = CloneEntity_Recursive_(_allocedUid, _srcEntity);

    // set parent
    const SceneNode& node = entity.GetSceneNode();
    if (node.HasParent())
    {
        entity.GetSceneNode().SetParent(node.GetParent());
    }

    return entity;
}

Entity Scene::CloneEntity_Recursive_(
    const UID _allocedUid,
    Entity    _srcEntity)
{
    JAM_ASSERT(_srcEntity, "Entity is not valid");
    JAM_ASSERT(_srcEntity != m_root, "Cannot clone root entity");

    // 새로운 엔티티 생성
    Entity entity = CreateEntity_(_allocedUid, {});

    // 컴포넌트 카피
    const ReflectionDB& db = ReflectionDB::GetInstance();
    for (const ComponentMetadata& meta: db.GetComponentMetadataContainer() | std::views::values)
    {
        if (meta.Has(_srcEntity))
        {
            // scene component do not trivial copy
            if (meta.typeHash == HashOf<SceneComponent>())
            {
                entity.ModifyWorldTransform() = _srcEntity.GetWorldTransform();
            }
            else
            {
                const void* pSrcValue = meta.Get(_srcEntity);
                meta.CreateByCopy(entity, pSrcValue);
            }
        }
    }

    // 자식들도 재귀적으로 클론
    const SceneNode& srcNode  = _srcEntity.GetSceneNode();
    Entity           srcChild = srcNode.GetFirstChild();
    while (srcChild)
    {
        const UID uid   = m_uidGenerator.Alloc();
        Entity    child = CloneEntity_Recursive_(uid, srcChild);   // clone child
        child.GetSceneNode().SetParent(entity);                    // set parent
        srcChild = srcChild.GetSceneNode().GetNextSibling();
    }

    // done
    return entity;
}

void Scene::CloneEntityDefered_(
    const UID    _allocedUid,
    const Entity _srcEntity)
{
    auto cloneEntity = [this, _allocedUid, _srcEntity]
    {
        CloneEntity_(_allocedUid, _srcEntity);
    };

    JAM_APP.SubmitCommandToMainThread(Bind(std::move(cloneEntity)));
}

Entity Scene::CreateEntitiesFrom_Recursive_(
    const UID                    _allocedUid,
    const ModelNode&             _node,
    const Vector<Ref<Mesh>>&     _meshes,
    const Vector<Ref<Material>>& _mats)
{
    // create root
    Entity entity = CreateEntity_(_allocedUid, _node.name);
    entity.ModifyLocalTransform().SetMatrix(_node.mtx);

    // create leaf
    const Int32 numEntities = NumericCast<Int32>(_node.meshIdxes.size());
    for (Int32 i = 0; i < numEntities; ++i)
    {
        // create leaf
        ScratchBuffer buf;
        buf.Format("{}_{}", _node.name, i);

        const StringView name        = buf.ToStringView();
        Entity           leaf        = CreateEntity(name);
        const Int32      meshIdx     = _node.meshIdxes[i];
        const Int32      materialIdx = _node.matIdxes[i];

        // create renderable component
        RenderableComponent rc = {};
        rc.pMesh               = _meshes[meshIdx];
        rc.pMaterial           = _mats[materialIdx];
        leaf.CreateComponent<RenderableComponent>(rc);

        // set parent
        leaf.GetSceneNode().SetParent(entity);
    }

    // create child nodes recursively
    for (const ModelNode& childNode: _node.children)
    {
        const UID uid   = m_uidGenerator.Alloc();
        Entity    child = CreateEntitiesFrom_Recursive_(uid, childNode, _meshes, _mats);
        child.GetSceneNode().SetParent(entity);
    }

    return entity;
}

Entity Scene::CreateEntitiesFrom_(
    const UID         _allocedUid,
    const Ref<Model>& _pModel)
{
    return CreateEntitiesFrom_Recursive_(_allocedUid, _pModel->GetRoot(), _pModel->GetMeshes(), _pModel->GetMaterials());
}

void Scene::CreateEntitiesFromDefered_(
    const UID         _allocedUid,
    const Ref<Model>& _pModel)
{
    auto createEntitiesFrom = [this, _allocedUid, _pModel]
    {
        CreateEntitiesFrom_(_allocedUid, _pModel);
    };

    JAM_APP.SubmitCommandToMainThread(Bind(std::move(createEntitiesFrom)));
}

void Scene::DestroyEntity_Recursive_(
    Entity _entity)
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
    JAM_APP.DispatchEvent(event);
}

void Scene::UpdateSceneGraph_()
{
    JAM_ASSERT(m_root, "Root entity is null");
    SceneNode& node = m_root.GetSceneNode();
    node.UpdateDirty_(m_transformModifiedEntities);   // collect modified nodes
}

}   // namespace Jam
