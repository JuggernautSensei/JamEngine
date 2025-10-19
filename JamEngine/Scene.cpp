#include "pch.h"

#include "Scene.h"

#include "Application.h"
#include "Components.h"
#include "Entity.h"

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

const HashMap<UID, Entity>& Scene::GetEntities() const
{
    return m_entityMap;
}

Entity Scene::CreateEntity(const std::string_view _name)
{
    UID uid = m_uidGenerator.Alloc();
    return CreateEntity_(false, uid, _name);
}

Entity Scene::CreateEntity(const UID _uid, const std::string_view _name)
{
    UID uid = m_uidGenerator.Alloc(_uid);
    return CreateEntity_(false, uid, _name);
}

Entity Scene::GetEntity(UID _uid) const
{
    auto it = m_entityMap.find(_uid);
    JAM_ASSERT(it != m_entityMap.end(), "Entity with UID {} does not exist", _uid);
    return it->second;
}

Entity Scene::FindEntity(const UID _uid) const
{
    auto it = m_entityMap.find(_uid);
    if (it == m_entityMap.end())
    {
        return Entity::s_null;
    }

    return it->second;
}

Entity Scene::CloneEntity(const Entity _entity)
{
    const Entity newEntity = CreateEntity();
    for (UInt32 typeID: std::views::keys(m_registry.storage()))
    {
        if (component_db::Has(typeID, _entity))   // _entity가 해당 컴포넌트를 가지고 있는지 확인
        {
            void* component = component_db::Get(typeID, _entity);                // 컴포넌트 데이터 가져오기
            component_db::CreateOrReplaceByCopy(typeID, newEntity, component);   // 새 엔티티에 컴포넌트 복사
        }
    }

    return newEntity;
}

void Scene::DestroyEntity(const Entity _entity)
{
    JAM_ASSERT(_entity.IsValid(), "Entity is not valid");
    JAM_ASSERT(_entity != m_rootEntity, "Cannot delete root entity");

    struct EntityDeletor
    {
        void operator()() const
        {
            pScene->DestroyEntity_Recursive_(entity);
        }

        Scene* pScene;
        Entity entity;
    };

    Application& app = Application::GetInstance();
    app.SubmitCommand(EntityDeletor { this, _entity });
}

bool Scene::IsEntityValid(const Entity _entity) const
{
    return m_registry.valid(_entity.GetHandle());
}

Entity Scene::GetRootEntity() const
{
    return m_rootEntity;
}

Scene::Scene()
{
    UID rootUID  = m_uidGenerator.Alloc();
    m_rootEntity = CreateEntity_(true, rootUID, "Root");
}

void Scene::DestroyEntity_Recursive_(Entity _entity)
{
    JAM_ASSERT(_entity.IsValid(), "Entity is not valid");
    JAM_ASSERT(_entity != m_rootEntity, "Cannot delete root entity");

    // 자식들 모두 삭제
    HierarchyTransform& t     = _entity.GetHierarchyTransform();
    Entity              child = t.GetFirstChild();
    while (child.IsValid())
    {
        HierarchyTransform& childT    = child.GetHierarchyTransform();
        Entity              nextChild = childT.GetNextSibling();   // 다음 형제 엔티티 미리 저장
        DestroyEntity_Recursive_(child);                           // 재귀적으로 자식 삭제
        child = nextChild;
    }

    t.ReleaseParent();

    UID uid = _entity.GetUID();
    m_entityMap.erase(uid);                    // entity map에서 제거
    m_uidGenerator.Free(uid);                  // uid 반환
    m_registry.destroy(_entity.GetHandle());   // entt 엔티티 파괴

    // 이벤트 디스패치
    EntityDestroyedEvent event { uid };
    Application::GetInstance().DispatchEvent(event);
}

Entity Scene::CreateEntity_(const bool _bCreateRoot, UID _uid, const std::string_view _name)
{
    JAM_ASSERT(m_uidGenerator.IsAllocated(_uid), "UID {} is not allocated", _uid);

    // entt 엔티티 생성
    entt::entity handle = m_registry.create();
    Entity       entity { m_registry, handle, _uid };

    // entity map에 추가
    JAM_ASSERT(!m_entityMap.contains(_uid), "Entity with same UID already exists");
    m_entityMap.emplace(_uid, entity);

    // transform 컴포넌트 추가
    TransformComponent& transform = entity.CreateComponent<TransformComponent>();
    if (_bCreateRoot)
        transform.transform = HierarchyTransform { entity };   // 루트 엔티티는 자기 자신이 루트
    else
        transform.transform = HierarchyTransform { m_rootEntity, entity };

    // 필요시 이름 태그 추가
    if (!_name.empty())
    {
        TagComponent& tag = entity.CreateComponent<TagComponent>();
        tag.name          = _name;
    }

    // 이벤트 디스패치
    EntityCreatedEvent event { _uid };
    Application::GetInstance().DispatchEvent(event);
    return entity;
}

}   // namespace jam
