#pragma once
#include "Entity.h"
#include "IEditableComponent.h"
#include "ISerializeableComponent.h"

namespace jam
{

using CreateComponentCallback      = std::function<void(Entity&)>;                                      // (1) owner entity
using RemoveComponentCallback      = std::function<void(Entity&)>;                                      // (1) owner entity
using GetComponentCallback         = std::function<void*(const Entity&)>;                               // (1) owner entity, return nullptr if not exists
using HasComponentCallback         = std::function<bool(const Entity&)>;                                // (1) owner entity
using SerializeComponentCallback   = std::function<Json(const void*)>;                                  // (1) component data (not nullptr)
using DeserializeComponentCallback = std::function<void(const Json&, Scene*, const Entity&, void*)>;    // (1) DeserializeParameter, (2) component data (not nullptr)
using DrawComponentEditorCallback  = std::function<void(EditorLayer*, Scene*, const Entity&, void*)>;   // (1) DrawEditorParameter, (2) component data (not nullptr)

struct ComponentMeta
{
    std::string_view componentName;
    UInt32           componentHash;

    CreateComponentCallback      createComponentCallback      = nullptr;
    RemoveComponentCallback      removeComponentCallback      = nullptr;
    GetComponentCallback         getComponentCallback         = nullptr;
    HasComponentCallback         hasComponentCallback         = nullptr;
    SerializeComponentCallback   serializeComponentCallback   = nullptr;
    DeserializeComponentCallback deserializeComponentCallback = nullptr;
    DrawComponentEditorCallback  drawComponentEditorCallback  = nullptr;
};

class ComponentMetaManager
{
public:
    using MetaContainer = std::unordered_map<std::string_view, ComponentMeta>;
    using NameContainer = std::unordered_map<UInt32, std::string_view>;

    ComponentMetaManager() = delete;

    template<typename T>
    static void RegisterComponent();
    static void RegisterComponent(const ComponentMeta& _meta);

    NODISCARD static bool             IsRegistered(std::string_view _componentName);
    NODISCARD static std::string_view GetComponentNameByHash(UInt32 _componentHash);
    static void                       CreateComponent(std::string_view _componentName, Entity& _owner);
    static void                       RemoveComponent(std::string_view _componentName, Entity& _owner);
    static void*                      GetComponentOrNull(std::string_view _componentName, const Entity& _owner);
    NODISCARD static bool             HasComponent(std::string_view _componentName, const Entity& _owner);
    NODISCARD static Json             SerializeComponent(std::string_view _componentName, const Entity& _owner);
    static void                       DeserializeComponent(std::string_view _componentName, const Json& _json, Scene* _pScene, const Entity& _onwerEntity);
    static void                       DrawComponentEditor(std::string_view _componentName, EditorLayer* _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity);

    NODISCARD static const MetaContainer& GetMetaContainer();
};

template<typename T>
void ComponentMetaManager::RegisterComponent()
{
    ComponentMeta meta;
    meta.componentName           = NameOf<T>();
    meta.componentHash           = HashOf<T>();
    meta.createComponentCallback = [](Entity& owner)
    {
        owner.CreateComponent<T>();
    };
    meta.removeComponentCallback = [](const Entity& owner)
    {
        owner.RemoveComponent<T>();
    };
    meta.getComponentCallback = [](const Entity& owner) -> void*
    {
        if constexpr (std::is_void_v<decltype(owner.GetComponent<T>())>)
        {
            return nullptr;
        }
        else
        {
            T& ref = owner.GetComponent<T>();
            return static_cast<void*>(&ref);
        }
    };
    meta.hasComponentCallback = [](const Entity& owner)
    {
        return owner.HasComponent<T>();
    };
    if constexpr (std::is_base_of_v<ISerializableComponent<T>, T>)
    {
        static_assert(sizeof(T) > 1, "empty struct cannot be serialized");
        meta.serializeComponentCallback = [](const void* componentValue)
        {
            JAM_ASSERT(componentValue, "Component value must not be nullptr");
            return static_cast<const T*>(componentValue)->Serialize_Super();
        };
        meta.deserializeComponentCallback = [](const Json& _componentValueJson, Scene* _pScene, const Entity& _ownerEntity, void* _out_component)
        {
            JAM_ASSERT(_out_component, "Out component value must not be nullptr");
            static_cast<T*>(_out_component)->Deserialize_Super(_componentValueJson, _pScene, _ownerEntity);
        };
    }
    if constexpr (std::is_base_of_v<IEditableComponent<T>, T>)
    {
        meta.drawComponentEditorCallback = [](EditorLayer* _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity, void* _componentValue)
        {
            static_assert(sizeof(T) > 1, "empty struct cannot be drawn in edit panel");
            T* pComponent = static_cast<T*>(_componentValue);
            pComponent->DrawEditor_Super(_pEditorLayer, _pScene, _ownerEntity);
        };
    }

    RegisterComponent(meta);
}

#define JAM_COMPONENT(_componentType) \
    inline static const bool _jam_component_meta_registered_ = []() { \
        ComponentMetaManager::RegisterComponent<_componentType>();    \
        return true; }();

}   // namespace jam