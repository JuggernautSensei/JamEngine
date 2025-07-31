#pragma once
#include "IComponentSerializable.h"

namespace jam
{

using CreateComponentCallback      = std::function<void(Entity)>;                                   // (1) owner entity (2) component data (or nullptr)
using RemoveComponentCallback      = std::function<void(Entity)>;                                   // (1) owner entity
using HasComponentCallback         = std::function<bool(Entity)>;                                   // (1) owner entity
using SerializeComponentCallback   = std::function<Json(void*)>;                                    // (1) component data (not nullptr)
using DeserializeComponentCallback = std::function<void(const ComponentDeserializeData&, void*)>;   // (1) data for deserialization (json, scene, owner entity ...), (2) out component data (not nullptr)

struct ComponentMeta
{
    std::string_view componentName;
    UInt32           componentHash;

    CreateComponentCallback      createCallback;
    RemoveComponentCallback      removeCallback;
    HasComponentCallback         hasCallback;
    SerializeComponentCallback   serializeCallback;
    DeserializeComponentCallback deserializeCallback;
};

class ComponentMetaManager
{
public:
    ComponentMetaManager() = delete;

    static void RegisterComponent(const ComponentMeta& _meta);

    template<typename T>
    static void RegisterComponent()
    {
        ComponentMeta meta;
        meta.componentName  = NameOf<T>();
        meta.componentHash  = HashOf<T>();
        meta.createCallback = [](Entity owner)
        {
            owner.CreateCompoenent<T>();
        };
        meta.removeCallback = [](const Entity owner)
        {
            owner.RemoveComponent<T>();
        };
        meta.hasCallback = [](const Entity owner)
        {
            return owner.HasComponent<T>();
        };
        meta.serializeCallback = [](void* componentValue)
        {
            JAM_ASSERT(componentValue, "Component value must not be nullptr");
            if constexpr (std::is_base_of_v<IComponentSerializable<T>, T>)
            {
                return static_cast<T*>(componentValue)->SerializeComponent();
            }
            else
            {
                return Json::value_t::null;
            }
        };
        meta.deserializeCallback = [](const ComponentDeserializeData& _deserializeData, void* _out_component)
        {
            JAM_ASSERT(_out_component, "Out component value must not be nullptr");
            if constexpr (std::is_base_of_v<IComponentSerializable<T>, T>)
            {
                static_cast<T*>(_out_component)->DeserializeComponent(_deserializeData);
            }
        };
        RegisterComponent(meta);
    }

    NODISCARD static bool             IsRegistered(std::string_view _componentName);
    NODISCARD static std::string_view GetComponentName(UInt32 _componentHash);
    static void                       CreateComponent(std::string_view _componentName, Entity _owner);
    static void                       RemoveComponent(std::string_view _componentName, Entity _owner);
    NODISCARD static bool             HasComponent(std::string_view _componentName, Entity _owner);
    NODISCARD static Json             SerializeComponent(std::string_view _componentName, void* _componentValue);
    static void                       DeserializeComponent(std::string_view _componentName, const ComponentDeserializeData& _deserializeData, void* _out_componentValue);

    NODISCARD static std::ranges::ref_view<std::unordered_map<std::string_view, ComponentMeta>> GetComponentMetaView();
};

#define JAM_COMPONENT(_componentType)                                 \
private:                                                              \
    inline static const bool _jam_component_meta_registered_ = []() { \
        ComponentMetaManager::RegisterComponent<_componentType>();    \
        return true;                                                  \
    }();                                                              \
                                                                      \
public:

#define JAM_STATIC_ASSERT_IS_REGISTERED_COMPONENT(_componentType) \
    static_assert(requires {{_componentType::_jam_component_meta_registered_} -> std::same_as<bool> }, "Component '" #_componentType "' must be registered with JAM_COMPONENT macro");

}   // namespace jam