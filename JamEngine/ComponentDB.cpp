#include "pch.h"

#include "ComponentDB.h"
#include "StringUtilities.h"

namespace jam
{

struct ComponentDBContext
{
    HashMap<UInt32, ComponentInfo> componentInfoMap;
    HashMap<UInt32, UInt32>        typeIDToNameHashT;
};

static ComponentDBContext& GetComponentDBContext()
{
    static ComponentDBContext ctx;
    return ctx;
}

}   // namespace jam

namespace jam
{

void ComponentDB::Reflect(std::string_view _name, const ComponentInfo& info)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = HashOf(_name);
    JAM_ASSERT(!ctx.componentInfoMap.contains(nameHash), "Component '{}' is already registered", _name);

    ctx.componentInfoMap[nameHash]       = info;
    ctx.typeIDToNameHashT[info.typeHash] = nameHash;
}

void ComponentDB::Create(std::string_view _name, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.createFn, "Component '{}' does not have a create function", _name);
    info.createFn(_entity);
}

void ComponentDB::CreateByCopy(std::string_view _name, const Entity _entity, const void* _pComponent)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.createByCopyFn, "Component '{}' does not have a copy constructor function", _name);
    info.createByCopyFn(_entity, _pComponent);
}

void ComponentDB::CreateOrReplace(std::string_view _name, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.createOrReplaceFn, "Component '{}' does not have a create or replace function", _name);
    info.createOrReplaceFn(_entity);
}

void ComponentDB::CreateOrReplaceByCopy(std::string_view _name, const Entity _entity, const void* _pComponent)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.createOrReplaceByCopyFn, "Component '{}' does not have a copy constructor for create or replace function", _name);
    info.createOrReplaceByCopyFn(_entity, _pComponent);
}

void ComponentDB::Remove(std::string_view _name, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.removeFn, "Component '{}' does not have a remove function", _name);
    info.removeFn(_entity);
}

void* ComponentDB::Get(std::string_view _name, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.getFn, "Component '{}' does not have a get function", _name);
    return info.getFn(_entity);
}

bool ComponentDB::Has(std::string_view _name, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.hasFn, "Component '{}' does not have a has function", _name);
    return info.hasFn(_entity);
}

Json ComponentDB::Serialize(std::string_view _name, const void* _pComponent, const Scene* _pScene, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.serializeFn, "Component '{}' does not have a serialize function", _name);
    return info.serializeFn(_pComponent, _pScene, _entity);
}

eErrorCode ComponentDB::Deserialize(std::string_view _name, void* _pComponent, const Json& _json, const Scene* _pScene, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.deserializeFn, "Component '{}' does not have a deserialize function", _name);
    return info.deserializeFn(_pComponent, _json, _pScene, _entity);
}

void ComponentDB::DrawEditor(std::string_view _name, void* _pComponent, const Scene* _pScene, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    JAM_ASSERT(info.drawEditorFn, "Component '{}' does not have a draw editor function", _name);
    info.drawEditorFn(_pComponent, _pScene, _entity);
}

bool ComponentDB::IsSerializable(const std::string_view _name)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    return info.serializeFn != nullptr;
}

bool ComponentDB::IsEditable(const std::string_view _name)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 hash = HashOf(_name);
    const auto&  info = ctx.componentInfoMap.at(hash);
    return info.drawEditorFn != nullptr;
}

void ComponentDB::Create(UInt32 _typeID, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.createFn, "Component with type hash {} does not have a create function", _typeID);
    info.createFn(_entity);
}

void ComponentDB::CreateByCopy(UInt32 _typeID, const Entity _entity, const void* _pComponent)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.createByCopyFn, "Component with type hash {} does not have a copy constructor function", _typeID);
    info.createByCopyFn(_entity, _pComponent);
}

void ComponentDB::CreateOrReplace(UInt32 _typeID, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.createOrReplaceFn, "Component with type hash {} does not have a create or replace function", _typeID);
    info.createOrReplaceFn(_entity);
}

void ComponentDB::CreateOrReplaceByCopy(UInt32 _typeID, const Entity _entity, const void* _pComponent)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.createOrReplaceByCopyFn, "Component with type hash {} does not have a copy constructor for create or replace function", _typeID);
    info.createOrReplaceByCopyFn(_entity, _pComponent);
}

void ComponentDB::Remove(UInt32 _typeID, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.removeFn, "Component with type hash {} does not have a remove function", _typeID);
    info.removeFn(_entity);
}

void* ComponentDB::Get(UInt32 _typeID, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.getFn, "Component with type hash {} does not have a get function", _typeID);
    return info.getFn(_entity);
}

bool ComponentDB::Has(UInt32 _typeID, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.hasFn, "Component with type hash {} does not have a has function", _typeID);
    return info.hasFn(_entity);
}

Json ComponentDB::Serialize(UInt32 _typeID, const void* _pComponent, const Scene* _pScene, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.serializeFn, "Component with type hash {} does not have a serialize function", _typeID);
    return info.serializeFn(_pComponent, _pScene, _entity);
}

eErrorCode ComponentDB::Deserialize(UInt32 _typeID, void* _pComponent, const Json& _json, const Scene* _pScene, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.deserializeFn, "Component with type hash {} does not have a deserialize function", _typeID);
    return info.deserializeFn(_pComponent, _json, _pScene, _entity);
}

void ComponentDB::DrawEditor(UInt32 _typeID, void* _pComponent, const Scene* _pScene, const Entity _entity)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    JAM_ASSERT(info.drawEditorFn, "Component with type hash {} does not have a draw editor function", _typeID);
    info.drawEditorFn(_pComponent, _pScene, _entity);
}

bool ComponentDB::IsSerializable(const UInt32 _typeID)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    return info.serializeFn != nullptr;
}

bool ComponentDB::IsEditable(const UInt32 _typeID)
{
    auto& ctx = GetComponentDBContext();

    const UInt32 nameHash = ctx.typeIDToNameHashT.at(_typeID);
    const auto&  info     = ctx.componentInfoMap.at(nameHash);
    return info.drawEditorFn != nullptr;
}

const HashMap<UInt32, ComponentInfo>& ComponentDB::GetComponentInfos()
{
    return GetComponentDBContext().componentInfoMap;
}

}   // namespace jam::component_db
