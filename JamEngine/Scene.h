#pragma once
#include "UID.h"
#include "Entity.h"
#include "UIDAllocator.h"

namespace Jam
{

class Model;
class Mesh;
class Event;
class Camera;
class Material;
class Transform;
struct ModelNode;

struct RenderView
{
    FrameBufferHandle fbh              = kInvalidHandle;   // output framebuffer
    Viewport          viewport         = {};               // output viewport
    const Camera*     pCameraOrNull    = nullptr;          // output camera info (can be null)
    const Transform*  pTransformOrNull = nullptr;          // output camera transform (can be null)
};

class Scene
{
    friend class SceneManager;

public:
    virtual ~Scene() = default;

    Scene(const Scene&)                = delete;
    Scene& operator=(const Scene&)     = delete;
    Scene(Scene&&) noexcept            = delete;
    Scene& operator=(Scene&&) noexcept = delete;

    // scene runtime interface
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void OnUpdate(float _deltaSec) {}
    virtual void OnRender(const RenderView& _view) {}
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Event& _event) {}
    virtual void OnResize(Int32 _width, Int32 _height) {}

    // rtti
    virtual StringView GetTypeName() const = 0;
    virtual UInt64     GetTypeHash() const = 0;

    // scene buffers
    [[nodiscard]] virtual TextureHandle GetSceneDepthTexture() const;

    // scene graph
    void                                UpdateSceneGraph();
    [[nodiscard]] const Vector<Entity>& GetTransformModifiedEntities() const;

    // entity management
    Entity CreateEntity(StringView _nameOrEmpty = {});
    Entity CreateEntity(UID _uid, StringView _nameOrEmpty = {});
    Entity CreateEntitiesFromModel(const Ref<Model>& _pModel);
    Entity CloneEntity(Entity _entity);

    void DestroyEntity(Entity _entity);
    void ClearEntities();

    [[nodiscard]] Entity GetEntity(UID _uid) const;
    [[nodiscard]] Entity FindEntityOrNull(UID _uid) const;
    [[nodiscard]] bool   IsEntityValid(Entity _entity) const;

    // entity view
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
    [[nodiscard]] SizeT                       GetNumEntities() const;
    [[nodiscard]] const HashMap<UID, Entity>& GetEntities() const;

    static constexpr StringView kExtension = ".jsc";
    static constexpr StringView kFilter    = "*.jsc";

protected:
    Scene();

private:
    void Reset_();

    // create entity. create entity must support create with uid. so param _allocedUID is required.
    Entity CreateRootEntity_();
    Entity CreateEntity_(UID _allocedUID, StringView _nameOrEmpty);

    // clone entity
    Entity CloneEntity_(UID _allocedUID, Entity _srcEntity);
    Entity CloneEntity_Recursive_(UID _allocedUID, Entity _srcEntity);

    // create from model
    Entity CreateEntitiesByModel_(UID _allocedUID, const Ref<Model>& _pModel);
    Entity CreateEntitiesByModel_Recursive_(UID _allocedUID, const ModelNode& _node, const Vector<Ref<Mesh>>& _meshes, const Vector<Ref<Material>>& _mats);
    void   CreateEntitieDeferredFromModel_(UID _allocedUID, const Ref<Model>& _pModel);

    // destroy entity
    void DestroyEntity_Recursive_(Entity _entity);

    entt::registry       m_registry                  = {};
    HashMap<UID, Entity> m_entities                  = {};
    UIDAllocator         m_uidAllocator              = {};
    Entity               m_root                      = Entity::kNull;
    Vector<Entity>       m_transformModifiedEntities = {};
};

}   // namespace Jam
