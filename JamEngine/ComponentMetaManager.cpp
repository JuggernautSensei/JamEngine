#include "pch.h"

#include "ComponentMetaManager.h"

namespace
{

using namespace jam;

NODISCARD auto& GetMetaContainer()
{
    static ComponentMetaManager::MetaContainer s_container;
    return s_container;
}

NODISCARD auto& GetNameContainer()
{
    static ComponentMetaManager::NameContainer s_container;
    return s_container;
}

}   // namespace

namespace jam
{

void ComponentMetaManager::RegisterComponent(const ComponentMeta& _meta)
{
    // register component meta
    {
        auto& map = ::GetMetaContainer();
        JAM_ASSERT(!map.contains(_meta.componentName), "Component '{}' is already registered", _meta.componentName);

        auto [_, result] = map.emplace(_meta.componentName, _meta);
        JAM_ASSERT(result, "Failed to register component '{}'", _meta.componentName);
    }

    // register component hash to name mapping
    {
        auto& nameMap = GetNameContainer();
        JAM_ASSERT(!nameMap.contains(_meta.componentHash), "Component hash '{}' is already registered for component '{}'", _meta.componentHash, _meta.componentName);

        auto [_, result] = nameMap.emplace(_meta.componentHash, _meta.componentName);
        JAM_ASSERT(result, "Failed to register component hash '{}' for component '{}'", _meta.componentHash, _meta.componentName);
    }
}

const std::unordered_map<std::string_view, ComponentMeta>& ComponentMetaManager::GetMetaContainer()
{
    return ::GetMetaContainer();
}

bool ComponentMetaManager::IsRegistered(const std::string_view _componentName)
{
    auto& map = GetMetaContainer();
    return map.contains(_componentName);
}

std::string_view ComponentMetaManager::GetComponentNameByHash(UInt32 _componentHash)
{
    auto& nameMap = ::GetNameContainer();
    auto  it      = nameMap.find(_componentHash);
    JAM_ASSERT(it != nameMap.end(), "Component with hash '{}' is not registered", _componentHash);
    return it->second;
}

void ComponentMetaManager::CreateComponent(std::string_view _componentName, Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    auto& map = GetMetaContainer();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.createComponentCallback, "Create callback for component '{}' is not set", _componentName);
    meta.createComponentCallback(_owner);
}

void ComponentMetaManager::RemoveComponent(std::string_view _componentName, Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    auto& map = GetMetaContainer();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.removeComponentCallback, "Remove callback for component '{}' is not set", _componentName);
    meta.removeComponentCallback(_owner);
}

void* ComponentMetaManager::GetComponentOrNull(std::string_view _componentName, const Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    auto& map = GetMetaContainer();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.hasComponentCallback, "Has callback for component '{}' is not set", _componentName);
    return meta.getComponentOrNullCallback(_owner);
}

bool ComponentMetaManager::HasComponent(std::string_view _componentName, const Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    auto& map = GetMetaContainer();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.hasComponentCallback, "Has callback for component '{}' is not set", _componentName);
    return meta.hasComponentCallback(_owner);
}

Json ComponentMetaManager::SerializeComponent(std::string_view _componentName, const Entity& _owner)
{
    auto& map = GetMetaContainer();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.serializeComponentCallback, "Serialize callback for component '{}' is not set", _componentName);
    const void* componentValue = meta.getComponentOrNullCallback(_owner);
    JAM_ASSERT(componentValue, "Component '{}' is not attached to owner entity or serialize impossible component. may be empty struct", _componentName);
    return meta.serializeComponentCallback(componentValue);
}

void ComponentMetaManager::DeserializeComponent(std::string_view _componentName, const DeserializeParameter& _param)
{
    JAM_ASSERT(_param.pJson, "DeserializeParameter::pJson must not be nullptr");
    JAM_ASSERT(_param.pScene, "DeserializeParameter::pScene must not be nullptr");
    JAM_ASSERT(_param.pOnwerEntity, "DeserializeParameter::pOnwerEntity must not be nullptr");

    auto& map = GetMetaContainer();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.deserializeComponentCallback, "Deserialize callback for component '{}' is not set", _componentName);

    void* outComponentValue = meta.getComponentOrNullCallback(*_param.pOnwerEntity);
    JAM_ASSERT(outComponentValue, "Component '{}' is not attached to owner entity or deserialize impossible component. may be empty struct", _componentName);
    meta.deserializeComponentCallback(_param, outComponentValue);
}

void ComponentMetaManager::DrawComponentEditor(std::string_view _componentName, const DrawComponentEditorParameter& _parma)
{
    JAM_ASSERT(_parma.pEditorLayer, "DrawComponentEditorParameter::pEditorLayer must not be nullptr");
    JAM_ASSERT(_parma.pOwnerEntity, "DrawComponentEditorParameter::pOwnerEntity must not be nullptr");

    auto& map = GetMetaContainer();
    auto  it  = map.find(_componentName);
    JAM_ASSERT(it != map.end(), "Component '{}' is not registered", _componentName);
    const ComponentMeta& meta = it->second;
    JAM_ASSERT(meta.drawComponentEditorCallback, "Draw edit panel callback for component '{}' is not set", _componentName);
    void* componentValue = meta.getComponentOrNullCallback(*_parma.pOwnerEntity);
    JAM_ASSERT(componentValue, "Component '{}' is not attached to owner entity or draw edit impossible component. may be empty struct", _componentName);
    meta.drawComponentEditorCallback(_parma, componentValue);
}
}   // namespace jam
