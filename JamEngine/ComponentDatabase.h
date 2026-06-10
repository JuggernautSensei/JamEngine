#pragma once
#include "Entity.h"

namespace Jam
{

class ComponentDatabase
{

    template<typename T>
    void CreateComponent_(Entity _entity) const
    {
        _entity.CreateComponent<T>();
    }

    template<typename T>
    void CreateComponentDeferred_(Entity _entity) const
    {
        _entity.CreateComponentDeferred<T>();
    }

    template<typename T>
    void CreateComponentByCopy_(Entity _entity, const void* _pCmp)
    {
        const T& cmp = *static_cast<const T*>(_pCmp);
        _entity.CreateComponent<T>(cmp);
    }

    template<typename T>
    void CreateComponentByCopyDeferred_(Entity _entity, const void* _pCmp)
    {
        const T& cmp = *static_cast<const T*>(_pCmp);
        _entity.CreateComponentDeferred<T>(cmp);
    }

    template<typename T>
    void RemoveComponent_(Entity _entity) const
    {
        _entity.RemoveComponents<T>();
    }

    template<typename T>
    void RemoveComponentDeferred_(Entity _entity) const
    {
        _entity.RemoveComponentsDeferred<T>();
    }

    template<typename T>
    void* GetComponent_(Entity _entity) const
    {

        if constexpr (std::is_void_v<decltype(std::declval<Entity>().GetComponents<T>())>)
        {
            return nullptr;
        }
        else
        {
            return static_cast<void*>(&_entity.GetComponents<T>());
        }
    }

    template<typename T>
    bool HasComponent_(const Entity _entity) const
    {
        return _entity.HaveComponents<T>();
    }

    template<typename T>
    void Serialize_(const void* _pCmp, ArchiveWriter& _writer, Scene* _pScene, const Entity _entity)
    {
        const T& cmp = *static_cast<const T*>(_pCmp);
        cmp.Serialize(_writer, _pScene, _entity);
    }

    template<typename T>
    void Deserialize_(void* _pCmp, ArchiveReader& _reader, Scene* _pScene, const Entity _entity)
    {
        T& cmp = *static_cast<T*>(_pCmp);
        cmp.Deserialize(_reader, _pScene, _entity);
    }

    template<typename T>
    void DrawEditPanel_(void* _pCmp, Scene* _pScene, Entity _entity)
    {
        T& cmp = *static_cast<T*>(_pCmp);
        cmp.DrawEditPanel(_pScene, _entity);
    }

    template<typename T>
    UInt32 ENTTHashOf_()
    {
        return entt::type_hash<T>::value();
    }

public:
    struct Data
    {
        StringView typeName          = {};
        StringView editorDisplayName = {};
        UInt64     typeHash          = 0;
        UInt32     enttHash          = 0;

        Delegate<void(Entity)>                                      Create               = nullptr;
        Delegate<void(Entity)>                                      CreateDeferred       = nullptr;
        Delegate<void(Entity, const void*)>                         CreateByCopy         = nullptr;
        Delegate<void(Entity, const void*)>                         CreateByCopyDeferred = nullptr;
        Delegate<void(Entity)>                                      Remove               = nullptr;
        Delegate<void(Entity)>                                      RemoveDeferred       = nullptr;
        Delegate<void*(Entity)>                                     Get                  = nullptr;
        Delegate<bool(Entity)>                                      Has                  = nullptr;
        Delegate<void(const void*, ArchiveWriter&, Scene*, Entity)> Serialize            = nullptr;
        Delegate<void(void*, ArchiveReader&, Scene*, Entity)>       Deserialize          = nullptr;
        Delegate<void(void*, Scene*, Entity)>                       DrawEditPanel        = nullptr;

        bool bNonRemoveable         = false;
        bool bNonCreateableOnEditor = false;
    };

    [[nodiscard]] static ComponentDatabase& GetInstance();

    template<typename T>
    void Register()
    {
        JAM_STATIC_ASSERT_CHECK_COMPONENT_PROPERTY_VALIDATIONS(T);

        Data data                   = {};
        data.typeName               = NameOf<T>();
        data.typeHash               = HashOf<T>();
        data.editorDisplayName      = NameOf<T>();
        data.Create                 = Bind<&ComponentDatabase::CreateComponent_<T>>(this);
        data.CreateDeferred         = Bind<&ComponentDatabase::CreateComponentDeferred_<T>>(this);
        data.CreateByCopy           = Bind<&ComponentDatabase::CreateComponentByCopy_<T>>(this);
        data.CreateByCopyDeferred   = Bind<&ComponentDatabase::CreateComponentByCopyDeferred_<T>>(this);
        data.Get                    = Bind<&ComponentDatabase::GetComponent_<T>>(this);
        data.Has                    = Bind<&ComponentDatabase::HasComponent_<T>>(this);
        data.bNonRemoveable         = HasComponentPropertyFlag<T, eComponentPropertyFlags::NonRemoveable>;
        data.bNonCreateableOnEditor = HasComponentPropertyFlag<T, eComponentPropertyFlags::NonCreatableOnEditor>;

        if constexpr (!HasComponentPropertyFlag<T, eComponentPropertyFlags::NonRemoveable>)
        {
            data.Remove         = Bind<&ComponentDatabase::RemoveComponent_<T>>(this);
            data.RemoveDeferred = Bind<&ComponentDatabase::RemoveComponentDeferred_<T>>(this);
        }

        if constexpr (HasComponentPropertyFlag<T, eComponentPropertyFlags::Editable>)
        {
            data.DrawEditPanel = Bind<&ComponentDatabase::DrawEditPanel_<T>>(this);
        }

        if constexpr (HasComponentPropertyFlag<T, eComponentPropertyFlags::Serializable>)
        {
            data.Serialize   = Bind<&ComponentDatabase::Serialize_<T>>(this);
            data.Deserialize = Bind<&ComponentDatabase::Deserialize_<T>>(this);
        }

        if constexpr (HasComponentEditorDisplayName<T>)
        {
            data.editorDisplayName = GetComponentEditorDisplayName<T>();
        }
        else
        {
            data.editorDisplayName = NameOf<T>();   // fallback
        }

        const UInt64 typeHash = data.typeHash;
        JAM_ASSERT(data.typeName.Hash() == typeHash, "Type typeName hash does not match type hash for component '{}'", data.typeName);
        JAM_ASSERT(!m_componentDataMap.contains(typeHash), "Component with typeName '{}' is already registered", data.typeName);
        m_componentDataMap.emplace(typeHash, data);

        const UInt32 enttHash = ENTTHashOf_<T>();
        JAM_ASSERT(!m_enttHashToTypeHash.contains(enttHash), "Component with entt hash '{}' is already registered", enttHash);
        m_enttHashToTypeHash.emplace(enttHash, typeHash);   // for fast lookup by entt hash
    }

    [[nodiscard]] bool IsComponentRegistered(StringView _typeName) const;
    [[nodiscard]] bool IsComponentRegistered(UInt64 _typeHash) const;
    [[nodiscard]] bool IsComponentRegisteredByENTTHash(UInt32 _enttHash) const;

    [[nodiscard]] const Data& GetComponentData(UInt64 _typeHash) const;
    [[nodiscard]] const Data& GetComponentData(StringView _typeName) const;
    [[nodiscard]] const Data& GetComponentDataByENTTHash(UInt32 _enttHash) const;

    [[nodiscard]] const PreHashedMap<UInt64, Data>& GetComponentDataContainer() const;

private:
    static void DestroyInstance();

    PreHashedMap<UInt64, Data>   m_componentDataMap   = {};
    PreHashedMap<UInt32, UInt64> m_enttHashToTypeHash = {};   // only supported for components

    inline static ComponentDatabase* s_pInstance = nullptr;
    friend class Application;
};

namespace Details
{

    template<typename T>
    class ComponentRegister
    {
    public:
        ComponentRegister()
        {
            ComponentDatabase& db = ComponentDatabase::GetInstance();
            db.Register<T>();
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
    inline static Jam::Details::ComponentRegister<TCmp> JAM_COMPONENT_REGISTERED = {}; \
                                                                                       \
public:                                                                                \
    JAM_COMPONENT_ESSENTIAL_MARKER()
