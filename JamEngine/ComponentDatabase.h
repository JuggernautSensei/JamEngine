#pragma once
#include "Entity.h"
#include "ComponentMetadata.h"

namespace Jam
{

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
        const T& cmp = *static_cast<const T*>(_pCmp);
        _entity.CreateComponent<T>(cmp);
    }

    template<typename T>
    void CreateComponentByCopyDefered(Entity _entity, const void* _pCmp)
    {
        const T& cmp = *static_cast<const T*>(_pCmp);
        _entity.CreateComponentDefered<T>(cmp);
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
    void Serialize(const void* _pCmp, ArchiveWriter& _writer, Scene* _pScene, const Entity _entity)
    {
        const T& cmp = *static_cast<const T*>(_pCmp);
        cmp.Serialize(_writer, _pScene, _entity);
    }

    template<typename T>
    void Deserialize(void* _pCmp, ArchiveReader& _reader, Scene* _pScene, const Entity _entity)
    {
        T& cmp = *static_cast<T*>(_pCmp);
        cmp.Deserialize(_reader, _pScene, _entity);
    }

    template<typename T>
    void DrawEditPanel(void* _pCmp, Scene* _pScene, Entity _entity)
    {
        T& cmp = *static_cast<T*>(_pCmp);
        cmp.DrawEditPanel(_pScene, _entity);
    }

    template<typename T>
    UInt32 EnttHashOf()
    {
        return entt::type_hash<T>::value();
    }

}   // namespace Details

struct ComponentReflectionData
{
    StringView typeName          = {};
    StringView editorDisplayName = {};
    UInt32     typeHash          = 0;
    UInt32     enttHash          = 0;

    Delegate<void(Entity)>                                      Create              = nullptr;
    Delegate<void(Entity)>                                      CreateDefered       = nullptr;
    Delegate<void(Entity, const void*)>                         CreateByCopy        = nullptr;
    Delegate<void(Entity, const void*)>                         CreateByCopyDefered = nullptr;
    Delegate<void(Entity)>                                      Remove              = nullptr;
    Delegate<void(Entity)>                                      RemoveDefered       = nullptr;
    Delegate<void*(Entity)>                                     Get                 = nullptr;
    Delegate<bool(Entity)>                                      Has                 = nullptr;
    Delegate<void(const void*, ArchiveWriter&, Scene*, Entity)> Serialize           = nullptr;
    Delegate<void(void*, ArchiveReader&, Scene*, Entity)>       Deserialize         = nullptr;
    Delegate<void(void*, Scene*, Entity)>                       DrawEditPanel       = nullptr;

    bool bNonRemoveable      = false;
    bool bNonCreatableByUser = false;
};

class ComponentDatabase
{
public:
    [[nodiscard]] static ComponentDatabase& GetInstance();

    template<typename T>
    void Reflect()
    {
        JAM_STATIC_ASSERT_COMPONENT_PROPERTY_VALIDATION_CHECKS(T);

        ComponentReflectionData data = {};
        data.typeName                = NameOf<T>();
        data.typeHash                = HashOf<T>();
        data.editorDisplayName       = NameOf<T>();
        data.Create                  = Bind<Details::CreateComponent<T>>();
        data.CreateDefered           = Bind<Details::CreateComponentDefered<T>>();
        data.CreateByCopy            = Bind<Details::CreateComponentByCopy<T>>();
        data.CreateByCopyDefered     = Bind<Details::CreateComponentByCopyDefered<T>>();
        data.Get                     = Bind<Details::Get<T>>();
        data.Has                     = Bind<Details::Has<T>>();
        data.bNonRemoveable          = HasComponentPropertyFlag<T, eComponentPropertyFlags::NonRemoveable>;
        data.bNonCreatableByUser     = HasComponentPropertyFlag<T, eComponentPropertyFlags::NonCreatableByUser>;

        if constexpr (!HasComponentPropertyFlag<T, eComponentPropertyFlags::NonRemoveable>)
        {
            data.Remove        = Bind<Details::RemoveComponent<T>>();
            data.RemoveDefered = Bind<Details::RemoveComponentDefered<T>>();
        }

        if constexpr (HasComponentPropertyFlag<T, eComponentPropertyFlags::Editable>)
        {
            data.DrawEditPanel = Bind<Details::DrawEditPanel<T>>();
        }

        if constexpr (HasComponentPropertyFlag<T, eComponentPropertyFlags::Serializable>)
        {
            data.Serialize   = Bind<Details::Serialize<T>>();
            data.Deserialize = Bind<Details::Deserialize<T>>();
        }

        if constexpr (HasComponentMetadataEditorDisplayName<T>)
        {
            data.editorDisplayName = GetComponentMetadataEditorDisplayName<T>();
        }

        const UInt32 typeHash = data.typeHash;
        JAM_ASSERT(data.typeName.Hash32() == typeHash, "Type typeName hash does not match type hash for component '{}'", data.typeName);
        JAM_ASSERT(!m_componentRflData.contains(typeHash), "Component with typeName '{}' is already registered", data.typeName);
        m_componentRflData.emplace(typeHash, data);

        const UInt32 enttHash = Details::EnttHashOf<T>();
        JAM_ASSERT(!m_enttHashToTypeHash.contains(enttHash), "Component with entt hash '{}' is already registered", enttHash);
        m_enttHashToTypeHash.emplace(enttHash, typeHash);   // for fast lookup by entt hash
    }

    [[nodiscard]] bool IsComponentReflected(StringView _typeName) const;
    [[nodiscard]] bool IsComponentReflected(UInt32 _typeHash) const;
    [[nodiscard]] bool IsComponentReflectedByEnttHash(UInt32 _enttHash) const;

    [[nodiscard]] const ComponentReflectionData& GetComponentReflectionByEnttHash(UInt32 _enttHash) const;
    [[nodiscard]] const ComponentReflectionData& GetComponentReflection(UInt32 _typeHash) const;
    [[nodiscard]] const ComponentReflectionData& GetComponentReflection(StringView _typeName) const;

    [[nodiscard]] const CASHashMap<UInt32, ComponentReflectionData>& GetComponentReflectionContainer() const;

private:
    static void DestroyInstance();

    CASHashMap<UInt32, ComponentReflectionData> m_componentRflData   = {};
    CASHashMap<UInt32, UInt32>                  m_enttHashToTypeHash = {};   // only supported for components

    inline static ComponentDatabase* s_pInstance = nullptr;
    friend class Application;
};

namespace Details
{

    template<typename T>
    class ComponentReflector
    {
    public:
        ComponentReflector()
        {
            ComponentDatabase::GetInstance().Reflect<T>();
        }
    };

}   // namespace Details

[[nodiscard]] inline ComponentDatabase& GetComponentDatabase()
{
    return ComponentDatabase::GetInstance();
}

}   // namespace Jam

#define JAM_COMPONENT_BODY(TCmp, ...)                                                  \
private:                                                                               \
    inline static Jam::Details::ComponentReflector<TCmp> JAM_COMPONENT_REFLECTED = {}; \
                                                                                       \
public:                                                                                \
    JAM_COMPONENT_METADATA_ESSENTIAL_MARKER()
