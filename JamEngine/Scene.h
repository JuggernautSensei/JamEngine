#pragma once
#include "Entity.h"
#include "Environment.h"
#include "RenderView.h"
#include "UID.h"

namespace jam
{

class Model;
class Mesh;
class Event;
class Material;
class Transform;
struct ModelNode;

class Scene : public RefCounted
{
    friend class SceneManager;

public:
    ~Scene() override = default;

    Scene(const Scene&)                = delete;
    Scene& operator=(const Scene&)     = delete;
    Scene(Scene&&) noexcept            = delete;
    Scene& operator=(Scene&&) noexcept = delete;

    // scene runtime interface
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void OnUpdate(float _deltaSec) {}
    virtual void OnRender(const RenderView& _desc) {}
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Event& _event) {}
    virtual void OnResize(Int32 _width, Int32 _height) {}

    // scene buffer
    [[nodiscard]] virtual TextureHandle GetDepthTexture() const;

    // item management
    Entity CreateEntity(StringView _nameOrEmpty = {});
    Entity CreateEntity(UID _uid, StringView _nameOrEmpty = {});
    UID    CreateEntityDefered(StringView _nameOrEmpty = {});             // returns the UID of the created entity. UID is invalid this frame, valid next frame
    UID    CreateEntityDefered(UID _uid, StringView _nameOrEmpty = {});   // returns the UID of the created entity. UID is invalid this frame, valid next frame

    Entity CloneEntity(Entity _entity);
    UID    CloneEntityDefered(Entity _entity);   // returns the UID of the cloned entity. UID is invalid this frame, valid next frame

    void DestroyEntity(Entity _entity);
    void DestroyEntityDefered(Entity _entity);

    [[nodiscard]] Entity GetEntity(UID _uid) const;
    [[nodiscard]] Entity FindEntity(UID _uid) const;   // 해당 UID를 가진 엔티티가 없으면 null 엔티티 반환
    [[nodiscard]] bool   IsEntityValid(Entity _entity) const;

    Entity CreateEntitiesFrom(const Ref<Model>& _pModel);
    UID    CreateEntitiesDeferedFrom(const Ref<Model>& _pModel);   // returns the UID of the root entity. UID is invalid this frame, valid next frame

    // item iteration
    template<typename... Args>
    [[nodiscard]] decltype(auto) View()
    {
        return m_registry.view<Args...>();
    }

    template<typename... Args>
    [[nodiscard]] decltype(auto) View() const
    {
        return m_registry.view<Args...>();
    }

    template<typename... Args>
    [[nodiscard]] decltype(auto) Each()
    {
        return m_registry.view<Args...>().each();
    }

    template<typename... Args>
    [[nodiscard]] decltype(auto) Each() const
    {
        return m_registry.view<Args...>().each();
    }

    [[nodiscard]] const entt::registry& GetRegistry() const;
    [[nodiscard]] entt::registry&       GetRegistry();

    // item
    [[nodiscard]] Entity                      GetRootEntity() const;
    [[nodiscard]] Int32                       GetNumEntities() const;
    [[nodiscard]] const HashMap<UID, Entity>& GetEntities() const;
    [[nodiscard]] Vector<Entity>              GetModifiedTransformEntities() const;   // 해당 배열은 OnRender 함수 호출 직전에 초기화됨

    [[nodiscard]] virtual StringView GetTypeName() const = 0;

protected:
    Scene();

private:
    // create entity. create entity must support create with uid. so param _allocedUid is required.
    Entity CreateRootEntity_();
    Entity CreateEntity_(UID _allocedUid, StringView _nameOrEmpty);
    void   CreateEntityDefered_(UID _allocedUid, StringView _nameOrEmpty);

    // clone entity
    Entity CloneEntity_(UID _allocatedUid, Entity _srcEntity);
    Entity CloneEntity_Recursive_(UID _allocatedUid, Entity _srcEntity);
    void   CloneEntityDefered_(UID _allocatedUid, Entity _srcEntity);

    // create from model
    Entity CreateEntitiesFrom_Recursive_(UID _allocatedUid, const ModelNode& _node, const Vector<Ref<Mesh>>& _meshes, const Vector<Ref<Material>>& _mats);
    Entity CreateEntitiesFrom_(UID _allocatedUid, const Ref<Model>& _pModel);
    void   CreateEntitiesFromDefered_(UID _allocatedUid, const Ref<Model>& _pModel);

    // destroy entity
    void DestroyEntity_Recursive_(Entity _entity);

    // scene graph update
    void UpdateSceneGraph_();   // call before rendering

    entt::registry       m_registry                  = {};
    UIDGenerator         m_uidGenerator              = {};
    HashMap<UID, Entity> m_entities                  = {};
    Entity               m_root                      = Entity::s_null;
    Vector<Entity>       m_modifiedTransformEntities = {};

    constexpr static UID k_reservedUidForRoot = 0;
};

}   // namespace jam

#define JAM_SCENE(_class)                                                           \
public:                                                                             \
    constexpr static StringView k_typename = NameOf<_class>();                      \
    [[nodiscard]] StringView    GetTypeName() const override { return k_typename; } \
                                                                                    \
private:                                                                            \
    inline static bool _scene_reflect_ = []                                         \
    {                                                                               \
        jam::SceneDB::GetInstance().Reflect<_class>();                              \
        return true;                                                                \
    }()
