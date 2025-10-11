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

Json Scene::OnSerialize() const
{
    return {};
}

eErrorCode Scene::OnDeserialize(const Json& _json)
{
    JAM_UNUSED(_json);
    return eErrorCode::Ok;
}

Entity Scene::CreateEntity(const std::string_view _name)
{
    return CreateEntity(m_uidGenerator.Alloc(), _name);
}

Entity Scene::CreateEntity(UID _uid, const std::string_view _name)
{
    JAM_ASSERT(_uid != k_invalidUID, "Cannot create entity with invalid UID");
    JAM_ASSERT(!m_entityMap.contains(_uid), "Entity with same UID already exists");
    JAM_ASSERT(!m_uidGenerator.IsAllocated(_uid), "UID {} is already allocated", _uid);

    // uid 할당
    UID uid = m_uidGenerator.Alloc(_uid);

    // entt 엔티티 생성
    entt::entity handle = m_registry.create();
    Entity       entity { m_registry, handle, uid };

    // entity map에 추가
    JAM_ASSERT(!m_entityMap.contains(uid), "Entity with same UID already exists");
    m_entityMap.emplace(uid, entity);

    // transform 컴포넌트 추가
    TransformComponent& transform = entity.CreateComponent<TransformComponent>();
    transform.root                = m_rootEntity;
    transform.owner               = entity;

    // 필요시 이름 태그 추가
    if (!_name.empty())
    {
        TagComponent& tag = entity.CreateComponent<TagComponent>();
        tag.name          = _name;
    }

    return entity;
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

Entity Scene::CloneEntity(Entity _entity)
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
        void operator()()
        {
            pScene->DestroyEntity_Recursive_(entity);
        }

        Scene* pScene;
        Entity entity;
    };

    Application& app = Application::GetInstance();
    app.SubmitCommand(EntityDeletor { this, _entity });
}

bool Scene::IsEntityValid(Entity _entity) const
{
    return m_registry.valid(_entity.GetHandle());
}

Entity Scene::GetRootEntity() const
{
    return m_rootEntity;
}

void Scene::DestroyEntity_Recursive_(Entity& _entity)
{
    JAM_ASSERT(_entity.IsValid(), "Entity is not valid");
    JAM_ASSERT(_entity != m_rootEntity, "Cannot delete root entity");

    // 자식들 모두 삭제
    TransformComponent& entityT = _entity.GetTransform();
    Entity              child   = entityT.firstChild;
    while (child.IsValid())
    {
        TransformComponent& childT    = child.GetTransform();
        Entity              nextChild = childT.nextSibling;   // 다음 형제 엔티티 미리 저장
        DestroyEntity_Recursive_(child);                      // 재귀적으로 자식 삭제
        child = nextChild;
    }

    // 계층구조 정리
    Entity& parent = entityT.parent;
    JAM_ASSERT(parent.IsValid(), "parent is null");   // 부모는 항상 존재해야 함

    TransformComponent& parentT = parent.GetTransform();
    if (parentT.firstChild == _entity)
    {
        // 첫번째 자식인 경우
        parentT.firstChild = entityT.nextSibling;   // 첫 번째 자식을 다음 형제(혹은 null)로 변경
    }
    else
    {
        // 첫번째 자식이 아닌 경우
        JAM_ASSERT(entityT.prevSibling.IsValid(), "prevSibling is null");
        TransformComponent& prevSiblingT = entityT.prevSibling.GetTransform();
        prevSiblingT.nextSibling         = entityT.nextSibling;   // 이전 형제의 다음 형제를 현재 노드의 다음 형제로 변경
    }

    if (entityT.nextSibling.IsValid())
    {
        // 다음 형제가 있는 경우, 다음 형제의 이전 형제를 현재 노드의 이전 형제(혹은 null)로 변경
        TransformComponent& nextSiblingT = entityT.nextSibling.GetTransform();
        nextSiblingT.prevSibling         = entityT.prevSibling;
    }

    // 트랜스폼 컴포넌트 갱신
    entityT.transform.ReleaseParent();

    m_entityMap.erase(_entity.GetUID());       // entity map에서 제거
    m_uidGenerator.Free(_entity.GetUID());     // uid 반환
    m_registry.destroy(_entity.GetHandle());   // entt 엔티티 파괴
}

}   // namespace jam
