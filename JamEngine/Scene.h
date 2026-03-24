#pragma once
#include "Entity.h"
#include "RenderView.h"
#include "UID.h"
#include "UIDAllocator.h"

namespace Jam
{

class Model;
class Mesh;
class Event;
class Material;
class Transform;
struct ModelNode;

enum class eSceneDeserializeFlags : UInt32
{
    None                         = 0,
    SkipComponentDeserializeFail = 1 << 0,   // 역직렬화 실패한 컴포넌트를 제거하고 계속 진행. 기본은 역직렬화 중단
    SkipSceneTypeMismatch        = 1 << 1,   // 씬 타입이 불일치해도 역직렬화 시도. 기본은 역직렬화 중단
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
    virtual UInt32     GetTypeHash() const = 0;

    // scene buffer
    [[nodiscard]] virtual TextureHandle GetDepthTexture() const;

    // serialize
    [[nodiscard]] static Expect<Scoped<Scene>> LoadFromFile(StringView _path, Flags<eSceneDeserializeFlags> _flags = eSceneDeserializeFlags::None);
    [[nodiscard]] static Expect<Scoped<Scene>> Load(StringView _json, Flags<eSceneDeserializeFlags> _flags = eSceneDeserializeFlags::None);
    virtual void                               Serialize(ArchiveWriter& _writer);
    virtual Error                              Deserialize(ArchiveReader& _reader, Flags<eSceneDeserializeFlags> _flags = eSceneDeserializeFlags::None);

    // scene graph
    void                                UpdateSceneGraph();
    [[nodiscard]] const Vector<Entity>& GetTransformModifiedEntities() const;   // 해당 배열은 Render 함수 호출 직전에 초기화됨

    // item management
    Entity CreateEntity(StringView _nameOrEmpty = {});
    Entity CreateEntity(UID _uid, StringView _nameOrEmpty = {});
    UID    CreateEntityDefered(StringView _nameOrEmpty = {});             // returns the UID of the created entity. UID is invalid this frame, valid next frame
    UID    CreateEntityDefered(UID _uid, StringView _nameOrEmpty = {});   // returns the UID of the created entity. UID is invalid this frame, valid next frame

    Entity CloneEntity(Entity _entity);
    UID    CloneEntityDefered(Entity _entity);   // returns the UID of the cloned entity. UID is invalid this frame, valid next frame

    void DestroyEntity(Entity _entity);
    void DestroyEntityDefered(Entity _entity);
    void ClearEntities();

    [[nodiscard]] Entity GetEntity(UID _uid) const;
    [[nodiscard]] Entity FindEntityOrNull(UID _uid) const;
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

    [[nodiscard]] const Registry& GetRegistry() const;
    [[nodiscard]] Registry&       GetRegistry();

    // item
    [[nodiscard]] Entity                      GetRootEntity() const;
    [[nodiscard]] Int32                       GetNumEntities() const;
    [[nodiscard]] const HashMap<UID, Entity>& GetEntities() const;

    static constexpr StringView k_extension = ".jsc";
    static constexpr StringView k_filter    = "*.jsc";

protected:
    Scene();

private:
    void Reset_();

    // create entity. create entity must support create with uid. so param _allocedUid is required.
    Entity CreateRootEntity_();
    Entity CreateEntity_(UID _allocedUid, StringView _nameOrEmpty);
    void   CreateEntityDefered_(UID _allocedUid, StringView _nameOrEmpty);

    // clone entity
    Entity CloneEntity_(UID _allocedUid, Entity _srcEntity);
    Entity CloneEntity_Recursive_(UID _allocedUid, Entity _srcEntity);
    void   CloneEntityDefered_(UID _allocedUid, Entity _srcEntity);

    // create from model
    Entity CreateEntitiesFrom_Recursive_(UID _allocedUid, const ModelNode& _node, const Vector<Ref<Mesh>>& _meshes, const Vector<Ref<Material>>& _mats);
    Entity CreateEntitiesFrom_(UID _allocedUid, const Ref<Model>& _pModel);
    void   CreateEntitiesFromDefered_(UID _allocedUid, const Ref<Model>& _pModel);

    // destroy entity
    void DestroyEntity_Recursive_(Entity _entity);

    Registry             m_registry                  = {};
    HashMap<UID, Entity> m_entities                  = {};
    UIDAllocator         m_uidAllocator              = {};
    Entity               m_root                      = Entity::k_null;
    Vector<Entity>       m_transformModifiedEntities = {};
};

}   // namespace Jam
