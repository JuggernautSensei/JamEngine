#pragma once
#include "Entity.h"
#include "ComponentProperties.h"
#include "ComponentMetadata.h"

namespace Jam
{

class ArchiveReader;
class ArchiveWriter;

class Scene;

namespace Details
{

    template<typename T>
    void CreateComponent(Entity _entity)
    {
        _entity.CreateComponent<T>();
    }

    template<typename T>
    void CreateComponentDefered(Entity _entity)
    {
        _entity.CreateComponentDefered<T>();
    }

    template<typename T>
    void CreateComponentByCopy(Entity _entity, const void* _pCmp)
    {
        const T& component = *static_cast<const T*>(_pCmp);
        _entity.CreateComponent<T>(component);
    }

    template<typename T>
    void CreateComponentByCopyDefered(Entity _entity, const void* _pCmp)
    {
        const T& component = *static_cast<const T*>(_pCmp);
        _entity.CreateComponentDefered<T>(component);
    }

    template<typename T>
    void RemoveComponent(Entity _entity)
    {
        _entity.RemoveComponent<T>();
    }

    template<typename T>
    void RemoveComponentDefered(Entity _entity)
    {
        _entity.RemoveComponentDefered<T>();
    }

    template<typename T>
    void* Get(Entity _entity)
    {
        if constexpr (std::is_void_v<decltype(std::declval<Entity>().GetComponent<T>())>)
        {
            return nullptr;
        }
        else
        {
            return static_cast<void*>(&_entity.GetComponent<T>());
        }
    }

    template<typename T>
    bool Has(const Entity _entity)
    {
        return _entity.HasComponent<T>();
    }

    template<typename T>
    void Serialize(const void* _pComp, ArchiveWriter& _writer, const Ref<Scene>& _pScene, const Entity _entity)
    {
        const T& cmp = *static_cast<const T*>(_pComp);
        cmp.Serialize(_writer, _pScene, _entity);
    }

    template<typename T>
    void Deserialize(void* _pComp, ArchiveReader& _reader, const Ref<Scene>& _pScene, const Entity _entity)
    {
        T& cmp = *static_cast<T*>(_pComp);
        cmp.Deserialize(_reader, _pScene, _entity);
    }

    template<typename T>
    void DrawEditPanel(void* _pCmp, const Ref<Scene>& _pScene, Entity _entity)
    {
        T& cmp = *static_cast<T*>(_pCmp);
        cmp.DrawEditPanel(_pScene, _entity);
    }

    template<typename T>
    Ref<Scene> CreateScene()
    {
        return MakeRef<T>();
    }

    template<typename T>
    UInt32 EnttHashOf()
    {
        return entt::type_hash<T>::value();
    }

}   // namespace Details

struct ComponentMetadata
{
    StringView typeName          = {};
    StringView editorDisplayName = {};
    UInt32     typeHash          = 0;
    UInt32     enttHash          = 0;

    Delegate<void(Entity)>                                                 Create              = nullptr;
    Delegate<void(Entity)>                                                 CreateDefered       = nullptr;
    Delegate<void(Entity, const void*)>                                    CreateByCopy        = nullptr;
    Delegate<void(Entity, const void*)>                                    CreateByCopyDefered = nullptr;
    Delegate<void(Entity)>                                                 Remove              = nullptr;
    Delegate<void(Entity)>                                                 RemoveDefered       = nullptr;
    Delegate<void*(Entity)>                                                Get                 = nullptr;
    Delegate<bool(Entity)>                                                 Has                 = nullptr;
    Delegate<void(const void*, ArchiveWriter&, const Ref<Scene>&, Entity)> Serialize           = nullptr;
    Delegate<void(void*, ArchiveReader&, const Ref<Scene>&, Entity)>       Deserialize         = nullptr;
    Delegate<void(void*, const Ref<Scene>&, Entity)>                       Panel_EditComponent = nullptr;

    bool bCannotRemove         = false;
    bool bCannotCreateInEditor = false;
};

struct SceneMetadata
{
    StringView             typeName = {};
    UInt32                 typeHash = 0;
    Delegate<Ref<Scene>()> Create   = nullptr;
};

class ReflectionDB
{
public:
    [[nodiscard]] static ReflectionDB& GetInstance();

    template<typename T>
    void ReflectComponent()
    {
        ComponentMetadata meta   = {};
        meta.typeName            = NameOf<T>();
        meta.typeHash            = HashOf<T>();
        meta.editorDisplayName   = NameOf<T>();
        meta.Create              = Bind<Details::CreateComponent<T>>();
        meta.CreateDefered       = Bind<Details::CreateComponentDefered<T>>();
        meta.CreateByCopy        = Bind<Details::CreateComponentByCopy<T>>();
        meta.CreateByCopyDefered = Bind<Details::CreateComponentByCopyDefered<T>>();
        meta.Get                 = Bind<Details::Get<T>>();
        meta.Has                 = Bind<Details::Has<T>>();

        if constexpr (!IsComponentTagEnabled<T, eComponentTag::NonRemoveable>)
        {
            meta.Remove        = Bind<Details::RemoveComponent<T>>();
            meta.RemoveDefered = Bind<Details::RemoveComponentDefered<T>>();
        }

        if constexpr (EditableComponent<T>)
        {
            meta.Panel_EditComponent = Bind<Details::DrawEditPanel<T>>();
        }

        if constexpr (SerializableComponent<T>)
        {
            meta.Serialize   = Bind<Details::Serialize<T>>();
            meta.Deserialize = Bind<Details::Deserialize<T>>();
        }

        if constexpr (IsComponentTagEnabled<T, eComponentTag::NonRemoveable>)
        {
            meta.bCannotRemove = true;
        }

        if constexpr (IsComponentTagEnabled<T, eComponentTag::CannotCreateInEditor>)
        {
            meta.bCannotCreateInEditor = true;
        }

        if constexpr (HasComponentDisplayNameInEditor<T>)
        {
            meta.editorDisplayName = JAM_GET_COMPONENT_DISPLAY_NAME_IN_EDITOR(T);
        }

        const UInt32 typeHash = meta.typeHash;
        JAM_ASSERT(meta.typeName.Hash32() == typeHash, "Type typeName hash does not match type hash for component '{}'", meta.typeName);
        JAM_ASSERT(!m_componentMetadataContainer.contains(typeHash), "Component with typeName '{}' is already registered", meta.typeName);
        m_componentMetadataContainer.emplace(typeHash, meta);

        const UInt32 enttHash = Details::EnttHashOf<T>();
        JAM_ASSERT(!m_enttHashToTypeHashMap.contains(enttHash), "Component with entt hash '{}' is already registered", enttHash);
        m_enttHashToTypeHashMap.emplace(enttHash, typeHash);   // for fast lookup by entt hash
    }

    template<typename T>
    void ReflectScene()
    {
        SceneMetadata meta = {};
        meta.typeName      = NameOf<T>();
        meta.typeHash      = HashOf<T>();
        meta.Create        = Details::CreateScene<T>;

        const UInt32 typeKey = meta.typeHash;
        JAM_ASSERT(meta.typeName.Hash32() == typeKey, "Type typeName hash does not match type hash for scene '{}'", meta.typeName);
        JAM_ASSERT(!m_sceneMetadataContainer.contains(typeKey), "Scene with typeName '{}' is already registered", meta.typeName);
        m_sceneMetadataContainer.emplace(typeKey, meta);
    }

    [[nodiscard]] bool IsComponentReflectedByEnttHash(UInt32 _enttHash) const;
    [[nodiscard]] bool IsComponentReflected(UInt32 _typeHash) const;
    [[nodiscard]] bool IsSceneReflected(UInt32 _typeHash) const;

    [[nodiscard]] bool IsComponentReflected(StringView _typeName) const;
    [[nodiscard]] bool IsSceneReflected(StringView _typeName) const;

    [[nodiscard]] const ComponentMetadata& GetComponentMetadataByEnttHash(UInt32 _enttHash) const;
    [[nodiscard]] const ComponentMetadata& GetComponentMetadata(UInt32 _typeHash) const;
    [[nodiscard]] const SceneMetadata&     GetSceneMetadata(UInt32 _typeHash) const;

    [[nodiscard]] const ComponentMetadata& GetComponentMetadata(StringView _typeName) const;
    [[nodiscard]] const SceneMetadata&     GetSceneMetadata(StringView _typeName) const;

    [[nodiscard]] const HashMap<UInt32, ComponentMetadata>& GetComponentMetadataContainer() const;
    [[nodiscard]] const HashMap<UInt32, SceneMetadata>&     GetSceneMetadataContainer() const;

private:
    static void DestroyInstance();

    HashMap<UInt32, ComponentMetadata> m_componentMetadataContainer = {};
    HashMap<UInt32, SceneMetadata>     m_sceneMetadataContainer     = {};
    HashMap<UInt32, UInt32>            m_enttHashToTypeHashMap      = {};   // only supported for components

    inline static ReflectionDB* s_pInstance = nullptr;
    friend class Application;
};

}   // namespace Jam

#define JAM_REFLECTION_DB \
    Jam::ReflectionDB::GetInstance()

namespace Jam
{

template<typename T>
struct ComponentReflector
{
    ComponentReflector()
    {
        JAM_REFLECTION_DB.ReflectComponent<T>();
    }
};

template<typename T>
struct SceneReflector
{
    SceneReflector()
    {
        JAM_REFLECTION_DB.ReflectScene<T>();
    }
};

}   // namespace Jam

#define JAM_REFLECTOR_VARIABLE_NAME __JAM_REFLECTOR__

#define JAM_REFLECT_COMPONENT(T)                                                                       \
private:                                                                                               \
    inline static Jam::ComponentReflector JAM_REFLECTOR_VARIABLE_NAME = Jam::ComponentReflector<T> {}; \
                                                                                                       \
public:                                                                                                \
    static constexpr StringView k_typeName = Jam::NameOf<T>();                                         \
    static constexpr UInt32     k_typehash = Jam::HashOf<T>()

#define JAM_REFLECT_SCENE(T)                                                        \
public:                                                                             \
    static constexpr StringView k_typeName = Jam::NameOf<T>();                      \
    static constexpr UInt32     k_typeHash = Jam::HashOf<T>();                      \
    [[nodiscard]] StringView    GetTypeName() const override { return k_typeName; } \
    [[nodiscard]] UInt32        GetTypeHash() const override { return k_typeHash; } \
                                                                                    \
private:                                                                            \
    inline static Jam::SceneReflector JAM_REFLECTOR_VARIABLE_NAME = Jam::SceneReflector<T> {}
