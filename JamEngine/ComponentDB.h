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

    std::function<void(const Entity&)>                                       createFn                = nullptr;
    std::function<void(const Entity&, const std::any&)>                      createByCopyFn          = nullptr;
    std::function<void(const Entity&)>                                       createOrReplaceFn       = nullptr;
    std::function<void(const Entity&, const std::any&)>                      createOrReplaceByCopyFn = nullptr;
    std::function<void(const Entity&)>                                       removeFn                = nullptr;
    std::function<std::any(const Entity&)>                                   getFn                   = nullptr;
    std::function<bool(const Entity&)>                                       hasFn                   = nullptr;
    std::function<Json(const std::any&, const Scene*, const Entity&)>        serializeFn             = nullptr;
    std::function<void(std::any&, const Json&, const Scene*, const Entity&)> deserializeFn           = nullptr;
    std::function<void(std::any&, const Scene*, const Entity&)>              drawEditorFn            = nullptr;
};

namespace component_db
{
    namespace details
    {
        template<typename T>
        void Create(const Entity& e)
        {
            e.CreateComponent<T>();
        }

        template<typename T>
        void CreateByCopy(const Entity& e, const std::any& other)
        {
            const T& otherComponent = std::any_cast<const T&>(other);
            e.CreateComponent<T>(otherComponent);
        }

        template<typename T>
        void CreateOrReplace(const Entity& e)
        {
            e.CreateOrReplaceComponent<T>();
        }

        template<typename T>
        void CreateOrReplaceByCopy(const Entity& e, const std::any& other)
        {
            const T& otherComponent = std::any_cast<const T&>(other);
            e.CreateOrReplaceComponent<T>(otherComponent);
        }

        template<typename T>
        void Remove(const Entity& e)
        {
            e.RemoveComponent<T>();
        }

        template<typename T>
        std::any Get(const Entity& e)
        {
            return e.GetComponent<T>();
        }

        template<typename T>
        bool Has(const Entity& e)
        {
            return e.HasComponent<T>();
        }

        template<typename T>
        Json Serialize(const std::any& c, const Scene* s, const Entity& e)
        {
            const T& component = std::any_cast<const T&>(c);
            return component.Serialize(s, e);
        }

        template<typename T>
        void Deserialize(std::any& c, const Json& j, const Scene* s, const Entity& e)
        {
            T& component = std::any_cast<T&>(c);
            component.Deserialize(j, s, e);
        }

        template<typename T>
        void DrawEditor(std::any& c, const Scene* s, const Entity& e)
        {
            T& component = std::any_cast<T&>(c);
            component.DrawEditor(s, e);
        }

    }   // namespace details

    void Reflect(std::string_view _name, const ComponentInfo& info);

    template<typename T>
    void Reflect(const std::string_view _name)
    {
        ComponentInfo info;
        info.name                    = _name;
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

        Reflect(_name, info);
    }

    // 런타임 접근용 (이름)
    void     Create(std::string_view _name, const Entity& _entity);
    void     CreateByCopy(std::string_view _name, const Entity& _entity, const std::any& _component);
    void     CreateOrReplace(std::string_view _name, const Entity& _entity);
    void     CreateOrReplaceByCopy(std::string_view _name, const Entity& _entity, const std::any& _component);
    void     Remove(std::string_view _name, const Entity& _entity);
    std::any Get(std::string_view _name, const Entity& _entity);
    bool     Has(std::string_view _name, const Entity& _entity);
    Json     Serialize(std::string_view _name, const std::any& _component, const Scene* _pScene, const Entity& _entity);
    void     Deserialize(std::string_view _name, std::any& _component, const Json& _json, const Scene* _pScene, const Entity& _entity);
    void     DrawEditor(std::string_view _name, std::any& _component, const Scene* _pScene, const Entity& _entity);

    // 런타임 접근용 (타입 해시)
    void     Create(UInt32 _typeID, const Entity& _entity);
    void     CreateByCopy(UInt32 _typeID, const Entity& _entity, const std::any& _component);
    void     CreateOrReplace(UInt32 _typeID, const Entity& _entity);
    void     CreateOrReplaceByCopy(UInt32 _typeID, const Entity& _entity, const std::any& _component);
    void     Remove(UInt32 _typeID, const Entity& _entity);
    std::any Get(UInt32 _typeID, const Entity& _entity);
    bool     Has(UInt32 _typeID, const Entity& _entity);
    Json     Serialize(UInt32 _typeID, const std::any& _component, const Scene* _pScene, const Entity& _entity);
    void     Deserialize(UInt32 _typeID, std::any& _component, const Json& _json, const Scene* _pScene, const Entity& _entity);
    void     DrawEditor(UInt32 _typeID, std::any& _component, const Scene* _pScene, const Entity& _entity);

    const HashMap<UInt32, ComponentInfo>& GetAllComponentInfos();

}   // namespace component_db
}   // namespace jam

#define JAM_COMPONENT(_type) \
    inline static bool JAM_CONCAT(_jam_component_, __LINE__) = [] { \
        jam::component_db::Reflect<_type>(#_type); \
        return true; }()