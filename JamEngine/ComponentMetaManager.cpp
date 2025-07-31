#include "pch.h"

#include "ComponentMetaManager.h"

namespace
{

NODISCARD auto& GetComponentMetaMap()
{
    static std::unordered_map<std::string_view, jam::ComponentMeta> s_componentMetaMap;
    return s_componentMetaMap;
}

NODISCARD auto& GetComponentHashNameMap()
{
    static std::unordered_map<jam::UInt32, std::string_view> s_componentHashNameMap;
    return s_componentHashNameMap;
}

}   // namespace

namespace jam
{

void ComponentMetaManager::RegisterComponent(const ComponentMeta& _meta)
{
    // register component meta
    {
        auto& map = GetComponentMetaMap();
        JAM_ASSERT(!map.contains(_meta.componentName), "Component '{}' is already registered", _meta.componentName);

        auto [_, result] = map.emplace(_meta.componentName, _meta);
        JAM_ASSERT(result, "Failed to register component '{}'", _meta.componentName);
    }

    // register component hash to name mapping
    {
        auto& nameMap = GetComponentHashNameMap();
        JAM_ASSERT(!nameMap.contains(_meta.componentHash), "Component hash '{}' is already registered for component '{}'", _meta.componentHash, _meta.componentName);

        auto [_, result] = nameMap.emplace(_meta.componentHash, _meta.componentName);
        JAM_ASSERT(result, "Failed to register component hash '{}' for component '{}'", _meta.componentHash, _meta.componentName);
    }
}

std::ranges::ref_view<std::unordered_map<std::string_view, ComponentMeta>> ComponentMetaManager::GetComponentMetaView()
{
    auto& map = GetComponentMetaMap();
    return std::views::all(map);
}

bool ComponentMetaManager::IsRegistered(const std::string_view _componentName)
{
    auto& map = GetComponentMetaMap();
    return map.contains(_componentName);
}

std::string_view ComponentMetaManager::GetComponentName(UInt32 _componentHash)
{
    auto& nameMap = GetComponentHashNameMap();
    auto  it      = nameMap.find(_componentHash);
    JAM_ASSERT(it != nameMap.end(), "Component with hash '{}' is not registered", _componentHash);
    return it->second;
}

void ComponentMetaManager::CreateComponent(std::string_view _componentName, const Entity _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    auto& map = GetComponentMetaMap();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.createCallback, "Create callback for component '{}' is not set", _componentName);
    meta.createCallback(_owner);
}

void ComponentMetaManager::RemoveComponent(std::string_view _componentName, Entity _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    auto& map = GetComponentMetaMap();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.removeCallback, "Remove callback for component '{}' is not set", _componentName);
    meta.removeCallback(_owner);
}

bool ComponentMetaManager::HasComponent(std::string_view _componentName, const Entity _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    auto& map = GetComponentMetaMap();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.hasCallback, "Has callback for component '{}' is not set", _componentName);
    return meta.hasCallback(_owner);
}

Json ComponentMetaManager::SerializeComponent(std::string_view _componentName, void* _componentValue)
{
    JAM_ASSERT(_componentValue, "Component value must not be nullptr");
    auto& map = GetComponentMetaMap();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.serializeCallback, "Serialize callback for component '{}' is not set", _componentName);
    return meta.serializeCallback(_componentValue);
}

void ComponentMetaManager::DeserializeComponent(std::string_view _componentName, const ComponentDeserializeData& _deserializeData, void* _out_componentValue)
{
    JAM_ASSERT(_out_componentValue, "Out component value must not be nullptr");
    auto& map = GetComponentMetaMap();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.deserializeCallback, "Deserialize callback for component '{}' is not set", _componentName);
    meta.deserializeCallback(_deserializeData, _out_componentValue);
}

}   // namespace jam
