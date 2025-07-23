#pragma once
#include "Entity.h"

namespace jam
{

using CreateComponentCallback      = std::function<void(Entity, const void*)>;   // (1) owner entity (2) component data (or nullptr)
using RemoveComponentCallback      = std::function<void(Entity)>;                // (1) owner entity
using GetComponentCallback         = std::function<const void*(Entity)>;         // (1) owner entity
using HasComponentCallback         = std::function<bool(Entity)>;                // (1) owner entity
using SerializeComponentCallback   = std::function<Json(const void*)>;           // (1) component data (not nullptr)
using DeserializeComponentCallback = std::function<void(const Json&, void*)>;    // (1) json (2) out parameter component data (not nullptr)

struct ComponentMeta
{
    std::string_view componentName;
    UInt32           componentHash;

    CreateComponentCallback      createCallback;
    RemoveComponentCallback      removeCallback;
    GetComponentCallback         getCall'back;
    HasComponentCallback         hasCallback;
    SerializeComponentCallback   serializeCallback;
    DeserializeComponentCallback deserializeCallback;
};

class ComponentMetaManager
{
public:
    ComponentMetaManager() = delete;

    static void RegisterComponent(const ComponentMeta& _meta);

    NODISCARD static const void* GetComponent(std::string_view _componentName, Entity _owner);
    NODISCARD static bool        HasComponent(std::string_view _componentName, Entity _owner);
    static void                  CreateComponent(std::string_view _componentName, Entity _owner, const void* _data = nullptr);
    static void                  RemoveComponent(std::string_view _componentName, Entity _owner);
};

}   // namespace jam