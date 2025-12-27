#pragma once
#include "EditableComponent.h"
#include "Entity.h"
#include "SerializableComponent.h"

namespace jam
{

struct ComponentInfo
{
    std::string name        = {};
    std::string displayName = {};
    UInt32      hash        = 0;

    std::function<void(Entity)>              createFn       = nullptr;
    std::function<void(Entity, const void*)> createByCopyFn = nullptr;
    std::function<void(Entity)>              removeFn       = nullptr;
    std::function<void*(Entity)>             getFn          = nullptr;
    std::function<bool(Entity)>              hasFn          = nullptr;

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
    void CreateByCopy(Entity _entity, const void* _pComponent)
    {
        const T& component = *static_cast<const T*>(_pComponent);
        _entity.CreateComponent<T>(component);
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
    void DrawEditor(void* _pComponent, Ref<Scene>& _pScene, Entity _entity)
    {
        T& component = *static_cast<T*>(_pComponent);
        component.DrawEditor(_pScene, _entity);
    }

    template<typename T>
    std::string DisplayNameOf()
    {
        const StringView name = NameOf<T>();

        std::string result;
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
    using ContainerT           = HashMap<UInt32, ComponentInfo>;
    using TypeIdHashContainerT = HashMap<UInt32, UInt32>;

    [[nodiscard]] static ComponentDB& GetInstance();
    static void                       DestroyInstance();

    void Reflect(const ComponentInfo& info);

    template<typename T>
    void Reflect()
    {
        ComponentInfo info;
        info.name           = NameOf<T>();
        info.displayName    = details::DisplayNameOf<T>();
        info.hash           = HashOf<T>();
        info.createFn       = details::Create<T>;
        info.createByCopyFn = details::CreateByCopy<T>;
        info.getFn          = details::Get<T>;
        info.hasFn          = details::Has<T>;

        if constexpr (!std::is_base_of_v<CannotRemovableComponent<T>, T>)
        {
            info.removeFn = details::Remove<T>;
        }

        if constexpr (std::is_base_of_v<EditableComponent<T>, T>)
        {
            info.drawEditorFn = details::DrawEditor<T>;
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

    // by name
    void                     Create(StringView _name, Entity _entity) const;
    void                     CreateByCopy(StringView _name, Entity _entity, const void* _pComponent) const;
    void                     Remove(StringView _name, Entity _entity) const;
    [[nodiscard]] void*      Get(StringView _name, Entity _entity) const;
    [[nodiscard]] bool       Has(StringView _name, Entity _entity) const;
    void                     Serialize(StringView _name, const void* _pComponent, JsonWriter _writer, const Ref<Scene>& _pScene, Entity _entity) const;
    void                     Deserialize(StringView _name, void* _pComponent, JsonReader _json, const Ref<Scene>& _pScene, Entity _entity) const;
    void                     DrawEditor(StringView _name, void* _pComponent, Ref<Scene>& _pScene, Entity _entity) const;
    [[nodiscard]] bool       IsSerializable(StringView _name) const;
    [[nodiscard]] bool       IsEditable(StringView _name) const;
    [[nodiscard]] StringView GetDisplayName(StringView _name) const;
    [[nodiscard]] bool       IsReflected(StringView _name) const;

    // by type id ( = type hash. TypeOf<T>() )
    void                     Create(UInt32 _typeId, Entity _entity) const;
    void                     CreateByCopy(UInt32 _typeId, Entity _entity, const void* _pComponent) const;
    void                     Remove(UInt32 _typeId, Entity _entity) const;
    [[nodiscard]] void*      Get(UInt32 _typeId, Entity _entity) const;
    [[nodiscard]] bool       Has(UInt32 _typeId, Entity _entity) const;
    void                     Serialize(UInt32 _typeId, const void* _pComponent, JsonWriter _writer, const Ref<Scene>& _pScene, Entity _entity) const;
    void                     Deserialize(UInt32 _typeId, void* _pComponent, JsonReader _json, const Ref<Scene>& _pScene, Entity _entity) const;
    void                     DrawEditor(UInt32 _typeId, void* _pComponent, Ref<Scene>& _pScene, Entity _entity) const;
    [[nodiscard]] bool       IsSerializable(UInt32 _typeId) const;
    [[nodiscard]] bool       IsEditable(UInt32 _typeId) const;
    [[nodiscard]] StringView GetDisplayName(UInt32 _typeId) const;
    [[nodiscard]] StringView GetComponentName(UInt32 _typeId) const;
    [[nodiscard]] bool       IsReflected(UInt32 _typeId) const;

    [[nodiscard]] const ContainerT& GetContainer() const;

private:
    static ComponentDB* s_instance;

    ContainerT           m_infos;
    TypeIdHashContainerT m_typeToNameContainer;
};   // namespace component_db

}   // namespace jam

#define JAM_COMPONENT(_type) \
private:                     \
    inline static bool _component_reflect_ = [] { \
        jam::ComponentDB::GetInstance().Reflect<_type>(); \
        return true; }();
