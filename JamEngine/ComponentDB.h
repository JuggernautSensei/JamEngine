#pragma once
#include "EditableComponent.h"
#include "Entity.h"
#include "SerializableComponent.h"

namespace jam
{

struct ComponentInfo
{
    std::string typeName    = {};
    std::string displayName = {};
    UInt32      typeHash    = 0;

    std::function<void(Entity)>              createFn              = nullptr;
    std::function<void(Entity)>              createDeferedFn       = nullptr;
    std::function<void(Entity, const void*)> createByCopyFn        = nullptr;
    std::function<void(Entity, const void*)> createByCopyDeferedFn = nullptr;
    std::function<void(Entity)>              removeFn              = nullptr;
    std::function<void(Entity)>              removeDeferedFn       = nullptr;
    std::function<void*(Entity)>             getFn                 = nullptr;
    std::function<bool(Entity)>              hasFn                 = nullptr;

    std::function<void(const void*, JsonWriter, const Ref<Scene>&, Entity)> serializeFn   = nullptr;
    std::function<void(void*, JsonReader, const Ref<Scene>&, Entity)>       deserializeFn = nullptr;
    std::function<void(void*, Ref<Scene>&, Entity)>                         drawEditorFn  = nullptr;

    bool bCannotRemovable = false;
    bool bCannotCreatable = false;
};

namespace details
{
    template<typename T>
    void Create(Entity _entity)
    {
        _entity.CreateComponent<T>();
    }

    template<typename T>
    void CreateDefered(Entity _entity)
    {
        _entity.CreateComponentDefered<T>();
    }

    template<typename T>
    void CreateByCopy(Entity _entity, const void* _pCmp)
    {
        const T& component = *static_cast<const T*>(_pCmp);
        _entity.CreateComponent<T>(component);
    }

    template<typename T>
    void CreateByCopyDefered(Entity _entity, const void* _pCmp)
    {
        const T& component = *static_cast<const T*>(_pCmp);
        _entity.CreateComponentDefered<T>(component);
    }

    template<typename T>
    void Remove(Entity _entity)
    {
        _entity.RemoveComponent<T>();
    }

    template<typename T>
    void RemoveDefered(Entity _entity)
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
    void Serialize(const void* _pComponent, JsonWriter _writer, const Ref<Scene>& _pScene, Entity _entity)
    {
        const T& component = *static_cast<const T*>(_pComponent);
        component.Serialize(_writer, _pScene, _entity);
    }

    template<typename T>
    void Deserialize(void* _pComponent, JsonReader _json, const Ref<Scene>& _pScene, Entity _entity)
    {
        T& component = *static_cast<T*>(_pComponent);
        component.Deserialize(_json, _pScene, _entity);
    }

    template<typename T>
    void Panel_EditComponent(void* _pComponent, Ref<Scene>& _pScene, Entity _entity)
    {
        T& component = *static_cast<T*>(_pComponent);
        component.Panel_EditComponent(_pScene, _entity);
    }

    template<typename T>
    std::string DisplayNameOf()
    {
        const StringView name = NameOf<T>();

        std::string result = {};
        result.reserve(name.GetLength());

        for (Int32 i = 0; i < name.GetLength(); ++i)
        {
            const char c     = name[i];
            const char lower = static_cast<char>(std::tolower(c));
            if (std::isupper(c) && !result.empty())
            {
                result += ' ';   // add space
                result += lower;
            }
            else
            {
                result += lower;
            }
        }

        return result;
    }

}   // namespace details

class ComponentDB
{
public:
    using ComponentInfoMap = HashMap<UInt32, ComponentInfo>;   // [type name hash]
    using ComponentHashMap = HashMap<UInt32, UInt32>;          // [type id] = type name hash

    [[nodiscard]] static ComponentDB& GetInstance();
    static void                       DestroyInstance();

    void Reflect(const ComponentInfo& info);

    template<typename T>
    void Reflect()
    {
        ComponentInfo info;
        info.typeName              = NameOf<T>();
        info.displayName           = details::DisplayNameOf<T>();
        info.typeHash              = HashOf<T>();
        info.createFn              = details::Create<T>;
        info.createDeferedFn       = details::CreateDefered<T>;
        info.createByCopyFn        = details::CreateByCopy<T>;
        info.createByCopyDeferedFn = details::CreateByCopyDefered<T>;
        info.getFn                 = details::Get<T>;
        info.hasFn                 = details::Has<T>;

        if constexpr (!std::is_base_of_v<CannotRemovableComponent<T>, T>)
        {
            info.removeFn        = details::Remove<T>;
            info.removeDeferedFn = details::RemoveDefered<T>;
        }

        if constexpr (std::is_base_of_v<EditableComponent<T>, T>)
        {
            info.drawEditorFn = details::Panel_EditComponent<T>;
        }

        if constexpr (std::is_base_of_v<SerializableComponent<T>, T>)
        {
            info.serializeFn   = details::Serialize<T>;
            info.deserializeFn = details::Deserialize<T>;
        }

        if constexpr (std::is_base_of_v<CannotRemovableComponent<T>, T>)
        {
            info.bCannotRemovable = true;
        }

        if constexpr (std::is_base_of_v<CannotCreatableInEditorComponent<T>, T>)
        {
            info.bCannotCreatable = true;
        }

        Reflect(info);
    }

    [[nodiscard]] bool       IsReflected(StringView _name) const;
    [[nodiscard]] bool       IsReflected(UInt32 _typeId) const;
    [[nodiscard]] StringView GetComponentName(UInt32 _typeId) const;

    [[nodiscard]] const ComponentInfoMap& GetComponentInfoMap() const;
    [[nodiscard]] const ComponentInfo&    GetComponentInfo(StringView _name) const;
    [[nodiscard]] const ComponentInfo&    GetComponentInfo(UInt32 _typeId) const;

private:
    static ComponentDB* s_instance;

    ComponentInfoMap m_infoMap;
    ComponentHashMap m_typeToNameMap;
};   // namespace component_db

}   // namespace jam

#define JAM_COMPONENT(_type)                           \
private:                                               \
    inline static bool _component_reflect_ = [] { \
        jam::ComponentDB::GetInstance().Reflect<_type>(); \
        return true; }(); \
                                                       \
public: