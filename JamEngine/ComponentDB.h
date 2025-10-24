#pragma once
#include "EditableComponent.h"
#include "Entity.h"
#include "SerializableComponent.h"

namespace jam
{

struct ComponentInfo
{
    std::string name     = {};
    UInt32      typeHash = 0;

    std::function<void(Entity)>                                         createFn                = nullptr;
    std::function<void(Entity, const void*)>                            createByCopyFn          = nullptr;
    std::function<void(Entity)>                                         createOrReplaceFn       = nullptr;
    std::function<void(Entity, const void*)>                            createOrReplaceByCopyFn = nullptr;
    std::function<void(Entity)>                                         removeFn                = nullptr;
    std::function<void*(Entity)>                                        getFn                   = nullptr;
    std::function<bool(Entity)>                                         hasFn                   = nullptr;
    std::function<Json(const void*, const Scene*, Entity)>              serializeFn             = nullptr;
    std::function<eErrorCode(void*, const Json&, const Scene*, Entity)> deserializeFn           = nullptr;
    std::function<void(void*, const Scene*, Entity)>                    drawEditorFn            = nullptr;
};

namespace details
{
    template<typename T>
    void Create(Entity _entity)
    {
        _entity.CreateComponent<T>();
    }

    template<typename T>
    void CreateByCopy(Entity _entity, const void* _pComponent)
    {
        const T& component = *static_cast<const T*>(_pComponent);
        _entity.CreateComponent<T>(component);
    }

    template<typename T>
    void CreateOrReplace(Entity _entity)
    {
        _entity.CreateOrReplaceComponent<T>();
    }

    template<typename T>
    void CreateOrReplaceByCopy(Entity _entity, const void* _pComponent)
    {
        const T& component = *static_cast<const T*>(_pComponent);
        _entity.CreateOrReplaceComponent<T>(component);
    }

    template<typename T>
    void Remove(Entity _entity)
    {
        _entity.RemoveComponent<T>();
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
    Json Serialize(const void* _pComponent, const Scene* _pScene, Entity _entity)
    {
        const T& component = *static_cast<const T*>(_pComponent);
        return component.Serialize(_pScene, _entity);
    }

    template<typename T>
    eErrorCode Deserialize(void* _pComponent, const Json& _json, const Scene* _pScene, Entity _entity)
    {
        T& component = *static_cast<T*>(_pComponent);
        return component.Deserialize(_json, _pScene, _entity);
    }

    template<typename T>
    void DrawEditor(void* _pComponent, const Scene* _pScene, Entity _entity)
    {
        T& component = *static_cast<T*>(_pComponent);
        component.DrawEditor(_pScene, _entity);
    }

}   // namespace details

class ComponentDB
{
public:
    static void Reflect(std::string_view _name, const ComponentInfo& info);

    template<typename T>
    static void Reflect()
    {
        ComponentInfo info;
        info.name                    = NameOf<T>();
        info.typeHash                = HashOf<T>();
        info.createFn                = details::Create<T>;
        info.createByCopyFn          = details::CreateByCopy<T>;
        info.createOrReplaceFn       = details::CreateOrReplace<T>;
        info.createOrReplaceByCopyFn = details::CreateOrReplaceByCopy<T>;
        info.getFn                   = details::Get<T>;
        info.hasFn                   = details::Has<T>;

        if constexpr (!std::is_base_of_v<UnremovableComponent<T>, T>)
        {
            info.removeFn = details::Remove<T>;
        }

        if constexpr (std::is_base_of_v<SerializableComponent<T>, T>)
        {
            info.serializeFn   = details::Serialize<T>;
            info.deserializeFn = details::Deserialize<T>;
        }

        if constexpr (std::is_base_of_v<EditableComponent<T>, T>)
        {
            info.drawEditorFn = details::DrawEditor<T>;
        }

        Reflect(info.name, info);
    }

    // 런타임 접근용 (이름)
    static void                Create(std::string_view _name, Entity _entity);
    static void                CreateByCopy(std::string_view _name, Entity _entity, const void* _component);
    static void                CreateOrReplace(std::string_view _name, Entity _entity);
    static void                CreateOrReplaceByCopy(std::string_view _name, Entity _entity, const void* _pComponent);
    static void                Remove(std::string_view _name, Entity _entity);
    JAM_NODISCARD static void* Get(std::string_view _name, Entity _entity);
    JAM_NODISCARD static bool  Has(std::string_view _name, Entity _entity);
    JAM_NODISCARD static Json  Serialize(std::string_view _name, const void* _pComponent, const Scene* _pScene, Entity _entity);
    static eErrorCode          Deserialize(std::string_view _name, void* _pComponent, const Json& _json, const Scene* _pScene, Entity _entity);
    static void                DrawEditor(std::string_view _name, void* _pComponent, const Scene* _pScene, Entity _entity);
    JAM_NODISCARD static bool  IsSerializable(std::string_view _name);
    JAM_NODISCARD static bool  IsEditable(std::string_view _name);

    // 런타임 접근용 (타입 해시)
    static void                Create(UInt32 _typeID, Entity _entity);
    static void                CreateByCopy(UInt32 _typeID, Entity _entity, const void* _pComponent);
    static void                CreateOrReplace(UInt32 _typeID, Entity _entity);
    static void                CreateOrReplaceByCopy(UInt32 _typeID, Entity _entity, const void* _pComponent);
    static void                Remove(UInt32 _typeID, Entity _entity);
    JAM_NODISCARD static void* Get(UInt32 _typeID, Entity _entity);
    JAM_NODISCARD static bool  Has(UInt32 _typeID, Entity _entity);
    JAM_NODISCARD static Json  Serialize(UInt32 _typeID, const void* _pComponent, const Scene* _pScene, Entity _entity);
    static eErrorCode          Deserialize(UInt32 _typeID, void* _pComponent, const Json& _json, const Scene* _pScene, Entity _entity);
    static void                DrawEditor(UInt32 _typeID, void* _pComponent, const Scene* _pScene, Entity _entity);
    JAM_NODISCARD static bool  IsSerializable(UInt32 _typeID);
    JAM_NODISCARD static bool  IsEditable(UInt32 _typeID);

    JAM_NODISCARD static const HashMap<UInt32, ComponentInfo>& GetComponentInfos();

};   // namespace component_db

}   // namespace jam

#define JAM_COMPONENT(_type)                           \
private:                                               \
    inline static bool _component_reflect_ = [] { \
        jam::ComponentDB::Reflect<_type>(); \
        return true; }(); \
                                                       \
public:                                                \
    constexpr static bool _component_marker_ = true