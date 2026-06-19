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

namespace Jam
{

namespace
{
    constexpr UID kRootEntityUID { 0 };
}

TextureHandle Scene::GetSceneDepthTexture() const
{
    return kInvalidHandle;
}

Entity Scene::CreateEntity(
    const StringView _nameOrEmpty)
{
    const UID uid = m_uidAllocator.Alloc();
    return CreateEntity_(uid, _nameOrEmpty);
}

Entity Scene::CreateEntity(
    const UID        _uid,
    const StringView _nameOrEmpty)
{
    const UID uid = m_uidAllocator.Alloc(_uid);
    return CreateEntity_(uid, _nameOrEmpty);
}

Entity Scene::CloneEntity(
    const Entity _entity)
{
    const UID uid = m_uidAllocator.Alloc();
    return CloneEntity_(uid, _entity);
}

void Scene::DestroyEntity(
    const Entity _entity)
{
    DestroyEntity_Recursive_(_entity);
}

void Scene::ClearEntities()
{
    m_registry.clear();
    m_entities.clear();
    m_uidAllocator.Clear();
    m_transformModifiedEntities.clear();
    m_root = CreateRootEntity_();
}

Entity Scene::GetEntity(
    UID _uid) const
{
    JAM_ASSERT(_uid, "UID is invalid");
    const auto it = m_entities.find(_uid);
    JAM_ASSERT(it != m_entities.end(), "Entity with UID {} does not exist", _uid);
    return it->second;
}

Entity Scene::FindEntityOrNull(
    const UID _uid) const
{
    if (!_uid.IsValid())
    {
        return Entity::kNull;
    }

    const auto it = m_entities.find(_uid);
    if (it == m_entities.end())
    {
        return Entity::kNull;
    }

    return it->second;
}

bool Scene::IsEntityValid(
    const Entity _entity) const
{
    return m_registry.valid(_entity.GetHandle());
}

Entity Scene::CreateEntitiesFromModel(
    const Ref<Model>& _pModel)
{
    JAM_ASSERT(_pModel, "Model is null");
    const UID uid = m_uidAllocator.Alloc();
    return CreateEntitiesByModel_(uid, _pModel);
}

const entt::registry& Scene::GetRegistry() const
{
    return m_registry;
}

entt::registry& Scene::GetRegistry()
{
    return m_registry;
}

Entity Scene::GetRootEntity() const
{
    JAM_ASSERT(m_root, "Root entity is null");
    return m_root;
}

SizeT Scene::GetNumEntities() const
{
    return m_entities.size();
}

const HashMap<UID, Entity>& Scene::GetEntities() const
{
    return m_entities;
}

const Vector<Entity, eastl::allocator>& Scene::GetTransformModifiedEntities() const
{
    return m_transformModifiedEntities;
}

Scene::Scene()
{
    m_root = CreateRootEntity_();
}

void Scene::Reset_()
{
    ClearEntities();
}

Entity Scene::CreateRootEntity_()
{
    JAM_ASSERT(!m_root, "Root entity already exists");
    JAM_ASSERT(!m_uidAllocator.IsAllocated(kRootEntityUID), "Root entity reserved UID is already allocated");

    // create entt handle
    const UID          uid    = m_uidAllocator.Alloc(kRootEntityUID);
    const entt::entity handle = m_registry.create();
    Entity             entity = { m_registry, handle, uid };

    // entity map
    JAM_ASSERT(!m_entities.contains(uid), "Entity with same UID already exists");
    m_entities.emplace(uid, entity);

    // add transformHierarchy component (essential component)
    entity.CreateComponent<SceneComponent>(SceneNode { entity });

    // create tag component
    entity.CreateComponent<TagComponent>("scene root");

    // dispatch event
    Application&       app = GetApp();
    EntityCreatedEvent event { entity };
    app.DispatchEvent(event);
    return entity;
}

Entity Scene::CreateEntity_(
    UID        _allocedUID,
    StringView _nameOrEmpty)
{
    JAM_ASSERT(m_uidAllocator.IsAllocated(_allocedUID), "UID {} is not allocated", _allocedUID);

    // create entt handle
    const entt::entity handle = m_registry.create();
    Entity             entity = { m_registry, handle, _allocedUID };

    // entity map
    JAM_ASSERT(!m_entities.contains(_allocedUID), "Entity with same UID already exists");
    m_entities.emplace(_allocedUID, entity);

    // add transformHierarchy component (essential component)
    entity.CreateComponent<SceneComponent>(SceneNode { m_root, entity });

    // create tag component if need
    if (!_nameOrEmpty.IsEmpty())
    {
        entity.CreateComponent<TagComponent>(_nameOrEmpty);
    }

    // dispatch event
    Application&       app = GetApp();
    EntityCreatedEvent event { entity };
    app.DispatchEvent(event);
    return entity;
}

Entity Scene::CloneEntity_(
    const UID    _allocedUID,
    const Entity _srcEntity)
{
    // clone
    Entity entity = CloneEntity_Recursive_(_allocedUID, _srcEntity);

    // set parent
    const SceneNode& node = entity.GetSceneNode();
    if (node.HasParent())
    {
        entity.GetSceneNode().SetParent(node.GetParent());
    }

    return entity;
}

Entity Scene::CloneEntity_Recursive_(
    const UID _allocedUID,
    Entity    _srcEntity)
{
    JAM_ASSERT(_srcEntity, "Entity is not valid");
    JAM_ASSERT(_srcEntity != m_root, "Cannot clone root entity");

    // 새로운 엔티티 생성
    Entity entity = CreateEntity_(_allocedUID, {});

    // 컴포넌트 카피
    const ComponentDatabase& db = GetComponentDatabase();
    for (const ComponentDatabase::Data& data: db.GetComponentDataContainer() | std::views::values)
    {
        if (data.Has(_srcEntity))
        {
            // scene component do not trivial copy
            if (data.hash == HashOf<SceneComponent>())
            {
                entity.ModifyWorld() = _srcEntity.GetWorld();
            }
            else
            {
                const void* pSrcValue = data.Get(_srcEntity);
                data.CreateByCopy(entity, pSrcValue);
            }
        }
    }

    // 자식들도 재귀적으로 클론
    const SceneNode& srcNode  = _srcEntity.GetSceneNode();
    Entity           srcChild = srcNode.GetFirstChild();
    while (srcChild.IsValid())
    {
        const UID uid   = m_uidAllocator.Alloc();
        Entity    child = CloneEntity_Recursive_(uid, srcChild);   // clone child
        child.GetSceneNode().SetParent(entity);                    // set parent
        srcChild = srcChild.GetSceneNode().GetNextSibling();
    }

    // done
    return entity;
}

Entity Scene::CreateEntitiesByModel_Recursive_(
    const UID                    _allocedUID,
    const ModelNode&             _node,
    const Vector<Ref<Mesh>>&     _meshes,
    const Vector<Ref<Material>>& _mats)
{
    // create root
    Entity entity = CreateEntity_(_allocedUID, _node.name);
    entity.ModifyLocal().SetSRT(_node.localMtx);

    // create leaf
    const Int32 numEntities = static_cast<Int32>(_node.meshIdxes.size());
    for (Int32 i = 0; i < numEntities; ++i)
    {
        // create leaf
        const String str         = String::Formatted("{}_{}", _node.name, i);
        Entity       leaf        = CreateEntity(str);
        const Int32  meshIdx     = _node.meshIdxes[i];
        const Int32  materialIdx = _node.matIdxes[i];

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
        const UID uid   = m_uidAllocator.Alloc();
        Entity    child = CreateEntitiesByModel_Recursive_(uid, childNode, _meshes, _mats);
        child.GetSceneNode().SetParent(entity);
    }

    return entity;
}

Entity Scene::CreateEntitiesByModel_(
    const UID         _allocedUID,
    const Ref<Model>& _pModel)
{
    return CreateEntitiesByModel_Recursive_(_allocedUID, _pModel->GetRootNode(), _pModel->GetMeshes(), _pModel->GetMaterials());
}

void Scene::CreateEntitieDeferredFromModel_(
    const UID         _allocedUID,
    const Ref<Model>& _pModel)
{
    Application& app = GetApp();
    app.SubmitCommandToMainThread(Bind(
        [this, _allocedUID, _pModel] {
            CreateEntitiesByModel_(_allocedUID, _pModel);
        }));
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
    while (child.IsValid())
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
    m_uidAllocator.Free(uid);
    m_registry.destroy(_entity.GetHandle());   // destroy

    // 이벤트 디스패치
    EntityDestroyedEvent event { _entity };
    Application&         app = GetApp();
    app.DispatchEvent(event);
}

void Scene::UpdateSceneGraph()
{
    JAM_ASSERT(m_root, "Root entity is null");
    SceneNode& node = m_root.GetSceneNode();
    node.UpdateGraph(m_transformModifiedEntities);   // collect modified nodes
}

}   // namespace Jam
