#include "pch.h"

#include "Scene.h"

#include "Application.h"
#include "Entity.h"
#include "Macros.h"
#include "Model.h"
#include "RenderableComponent.h"
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

void Scene::UpdateSceneGraph_()
{
    JAM_ASSERT(m_root, "Root entity is null");
    SceneNode& node = m_root.GetSceneNode();
    node.UpdateDirty_(m_modifiedTransformEntities);   // collect modified nodes
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
    const UID uid = m_uidGenerator.Alloc();
    return CloneEntity_(uid, _entity);
}

void Scene::DestroyEntity(
    const Entity _entity)
{
    DestroyEntity_Recursive_(_entity);
}

void Scene::DestroyEntityDefered(
    const Entity _entity)
{
    Application& app = Application::GetInstance();
    app.SubmitCommand(JAM_BIND_METHOD(this, Scene::DestroyEntity_Recursive_, _entity));
}

UID Scene::CloneEntityDefered(
    const Entity _entity)
{
    const UID uid = m_uidGenerator.Alloc();
    CloneEntityDefered_(uid, _entity);
    return uid;
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
    Application::GetInstance().DispatchEvent(event);
}

Entity Scene::CreateEntitiesFrom_Recursive_(
    const UID                    _allocatedUid,
    const ModelNode&             _node,
    const Vector<Ref<Mesh>>&     _meshes,
    const Vector<Ref<Material>>& _mats)
{
    // create root
    Entity entity = CreateEntity_(_allocatedUid, _node.name);
    entity.ModifyLocalTransform().SetMatrix(_node.mtx);

    // create leaf
    const Int32 numEntities = _node.meshIdxes.size();
    for (Int32 i = 0; i < numEntities; ++i)
    {
        // create leaf
        const StringView name        = GlobalBuffer::FormatTmp("{}_{}", _node.name, i);
        Entity           leaf        = CreateEntity(name);
        const Int32      meshIdx     = _node.meshIdxes[i];
        const Int32      materialIdx = _node.matIdxes[i];

        // create renderable component
        RenderableComponent rc;
        rc.renderable.pMesh     = _meshes[meshIdx];
        rc.renderable.pMaterial = _mats[materialIdx];
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
    const UID         _allocatedUid,
    const Ref<Model>& _pModel)
{
    return CreateEntitiesFrom_Recursive_(_allocatedUid, _pModel->GetRootNode(), _pModel->GetMeshs(), _pModel->GetMaterials());
}

void Scene::CreateEntitiesFromDefered_(
    const UID         _allocatedUid,
    const Ref<Model>& _pModel)
{
    Application& app = Application::GetInstance();
    app.SubmitCommand(JAM_BIND_METHOD(this, Scene::CreateEntitiesFrom_, _allocatedUid, _pModel));
}

Entity Scene::CloneEntity_Recursive_(
    const UID _allocatedUid,
    Entity    _srcEntity)
{
    JAM_ASSERT(_srcEntity, "Entity is not valid");
    JAM_ASSERT(_srcEntity != m_root, "Cannot clone root entity");

    // 새로운 엔티티 생성
    Entity entity = CreateEntity_(_allocatedUid, {});

    // 컴포넌트 카피
    const ComponentDB& db = ComponentDB::GetInstance();
    for (const ComponentInfo& info: db.GetComponentInfoMap() | std::views::values)
    {
        // scene component do not trivial copy
        // just copy transform
        if (info.typeHash == HashOf<SceneComponent>())
        {
            entity.ModifyWorldTransform() = _srcEntity.GetWorldTransform();
        }
        else
        {
            if (info.hasFn(_srcEntity))
            {
                const void* pCmpData = info.getFn(_srcEntity);
                info.createByCopyFn(entity, pCmpData);
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
    const UID    _allocatedUid,
    const Entity _srcEntity)
{
    Application& app = Application::GetInstance();
    app.SubmitCommand(JAM_BIND_METHOD(this, Scene::CloneEntity_, _allocatedUid, _srcEntity));
}

void Scene::CreateEntityDefered_(
    const UID        _allocedUid,
    const StringView _nameOrEmpty)
{
    const std::string name = _nameOrEmpty.ToString();
    Application&      app  = Application::GetInstance();
    app.SubmitCommand(JAM_BIND_METHOD(this, Scene::CreateEntity_, _allocedUid, name));
}

Entity Scene::CloneEntity_(
    const UID    _allocatedUid,
    const Entity _srcEntity)
{
    // clone
    Entity entity = CloneEntity_Recursive_(_allocatedUid, _srcEntity);

    // set parent
    const SceneNode& node = entity.GetSceneNode();
    if (node.HasParent())
    {
        entity.GetSceneNode().SetParent(node.GetParent());
    }
    return entity;
}

Entity Scene::CreateEntity_(
    UID        _allocedUid,
    StringView _nameOrEmpty)
{
    JAM_ASSERT(m_uidGenerator.IsAllocated(_allocedUid), "UID {} is not allocated", _allocedUid);

    // create entt handle
    const entt::entity handle = m_registry.create();
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
