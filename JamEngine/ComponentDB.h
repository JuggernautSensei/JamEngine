#pragma once
#include "EditableComponent.h"
#include "Entity.h"
#include "Json.h"
#include "SerializableComponent.h"

namespace jam
{

struct ComponentInfo
{
    std::string name        = {};
    std::string displayName = {};
    UInt32      hash        = 0;

    std::function<void(Entity)>                                  createFn         = nullptr;
    std::function<void(Entity, const void*)>                     createByCopyFn   = nullptr;
    std::function<void(Entity)>                                  removeFn         = nullptr;
    std::function<void*(Entity)>                                 getFn            = nullptr;
    std::function<bool(Entity)>                                  hasFn            = nullptr;
    std::function<Json(const void*, const Ref<Scene>&, Entity)>  serializeFn      = nullptr;
    std::function<void(void*, const Json&, Ref<Scene>&, Entity)> deserializeFn    = nullptr;
    std::function<void(void*, Ref<Scene>&, Entity)>              drawEditorFn     = nullptr;
    bool                                                         bCannotRemovable = false;
    bool                                                         bCannotCreatable = false;
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
    Json Serialize(const void* _pComponent, const Ref<Scene>& _pScene, Entity _entity)
    {
        const T& component = *static_cast<const T*>(_pComponent);
        return component.Serialize(_pScene, _entity);
    }

    template<typename T>
    void Deserialize(void* _pComponent, const Json& _json, Ref<Scene>& _pScene, Entity _entity)
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

}   // namespace details

class ComponentDB
{
public:
    using ContainerT          = HashMap<UInt32, ComponentInfo>;
    using TypeToNameContainer = HashMap<UInt32, UInt32>;

    [[nodiscard]] static ComponentDB& GetInstance();
    static void                       DestroyInstance();

    void Reflect(const ComponentInfo& info);

    template<typename T>
    void Reflect()
    {
        ComponentInfo info;
        info.name           = NameOf<T>();
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

        // display name = after last '::' + lower case
        {
            const size_t      pos = info.name.rfind("::");
            const std::string tmp = (pos == std::string::npos) ? info.name : info.name.substr(pos + 2);
            info.displayName.reserve(tmp.size());
            for (char c: tmp)
            {
                const char lower = static_cast<char>(std::tolower(c));
                if (std::isupper(c) && !info.displayName.empty())
                {
                    info.displayName += ' ';
                    info.displayName += lower;
                }
                else
                {
                    info.displayName += lower;
                }
            }
        }

        Reflect(info);
    }

    // by name
    void                           Create(std::string_view _name, Entity _entity) const;
    void                           CreateByCopy(std::string_view _name, Entity _entity, const void* _pComponent) const;
    void                           Remove(std::string_view _name, Entity _entity) const;
    [[nodiscard]] void*            Get(std::string_view _name, Entity _entity) const;
    [[nodiscard]] bool             Has(std::string_view _name, Entity _entity) const;
    [[nodiscard]] Json             Serialize(std::string_view _name, const void* _pComponent, const Ref<Scene>& _pScene, Entity _entity) const;
    void                           Deserialize(std::string_view _name, void* _pComponent, const Json& _json, Ref<Scene>& _pScene, Entity _entity) const;
    void                           DrawEditor(std::string_view _name, void* _pComponent, Ref<Scene>& _pScene, Entity _entity) const;
    [[nodiscard]] bool             IsSerializable(std::string_view _name) const;
    [[nodiscard]] bool             IsEditable(std::string_view _name) const;
    [[nodiscard]] std::string_view GetDisplayName(std::string_view _name) const;
    [[nodiscard]] bool             IsReflected(std::string_view _name) const;

    // by type id ( = type hash. TypeOf<T>() )
    void                           Create(UInt32 _typeID, Entity _entity) const;
    void                           CreateByCopy(UInt32 _typeID, Entity _entity, const void* _pComponent) const;
    void                           Remove(UInt32 _typeID, Entity _entity) const;
    [[nodiscard]] void*            Get(UInt32 _typeID, Entity _entity) const;
    [[nodiscard]] bool             Has(UInt32 _typeID, Entity _entity) const;
    [[nodiscard]] Json             Serialize(UInt32 _typeID, const void* _pComponent, const Ref<Scene>& _pScene, Entity _entity) const;
    void                           Deserialize(UInt32 _typeID, void* _pComponent, const Json& _json, Ref<Scene>& _pScene, Entity _entity) const;
    void                           DrawEditor(UInt32 _typeID, void* _pComponent, Ref<Scene>& _pScene, Entity _entity) const;
    [[nodiscard]] bool             IsSerializable(UInt32 _typeID) const;
    [[nodiscard]] bool             IsEditable(UInt32 _typeID) const;
    [[nodiscard]] std::string_view GetDisplayName(UInt32 _typeID) const;
    [[nodiscard]] std::string_view GetComponentName(UInt32 _typeID) const;
    [[nodiscard]] bool             IsReflected(UInt32 _typeID) const;

    [[nodiscard]] const ContainerT& GetContainer() const;

private:
    static ComponentDB* s_instance;

    ContainerT          m_infos;
    TypeToNameContainer m_typeToNameContainer;
};   // namespace component_db

}   // namespace jam

#define JAM_COMPONENT(_type)                           \
private:                                               \
    inline static bool _component_reflect_ = [] { \
        jam::ComponentDB::GetInstance().Reflect<_type>(); \
        return true; }(); \
                                                       \
public:                                                \
    constexpr static bool _component_marker_ = true