#pragma once
#include "Entity.h"
#include "GfxBase.h"
#include "UID.h"

// clang-format off
namespace jam { struct Camera; struct Transform; class Event; }
// clang-format on

namespace jam
{

class Scene
{
public:
    virtual ~Scene();

    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept;
    Scene& operator=(Scene&&) noexcept;

    virtual void OnEnter();
    virtual void OnExit();

    virtual void OnUpdate(float _deltaTime);
    virtual void OnRender(FrameBufferHandle _handle, const Camera& _camera, const Transform& _transform);
    virtual void OnEvent(Event& _event);

    virtual eErrorCode OnDeserialize(const Json& _js);
    virtual Json OnSerialize() const;

    void Clear();

    JAM_NODISCARD Entity CreateEntity(std::string_view _name = {});
    JAM_NODISCARD Entity CreateEntity(UID _uid, std::string_view _name = {});   // 특정 UID로 엔티티 생성, 이미 존재하는 UID면 assert 발생

    JAM_NODISCARD Entity GetEntity(UID _uid) const;
    JAM_NODISCARD Entity FindEntity(UID _uid) const;   // 해당 UID를 가진 엔티티가 없으면 null 엔티티 반환

    JAM_NODISCARD Entity CloneEntity(const Entity& _entity);
    void                 DestroyEntity(const Entity& _entity);
    JAM_NODISCARD bool   IsEntityValid(const Entity& _entity) const;

    template<typename... Args>
    JAM_NODISCARD decltype(auto) CreateView()
    {
        return m_registry.view<Args...>();
    }

    JAM_NODISCARD const entt::registry& GetRegistry() const;
    JAM_NODISCARD entt::registry& GetRegistry();

    JAM_NODISCARD const Entity& GetRootEntity() const;
    JAM_NODISCARD const HashMap<UID, Entity>& GetEntities() const;

private:
    Scene() = default;
    void DestroyEntity_Recursive_(Entity& _entity);

    entt::registry       m_registry     = {};
    Entity               m_rootEntity   = Entity::s_null;
    UIDGenerator         m_uidGenerator = {};
    HashMap<UID, Entity> m_entityMap    = {};
};

}   // namespace jam