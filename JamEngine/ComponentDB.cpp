#include "pch.h"

#include "ComponentDB.h"

#include "StringUtilities.h"
using namespace jam;

namespace
{

HashMap<UInt32, ComponentInfo> g_componentInfoMap;
HashMap<UInt32, UInt32>        g_typeIDToNameHashT;

}   // namespace

namespace jam
{
namespace component_db
{
    void Reflect(std::string_view _name, const ComponentInfo& info)
    {
        const UInt32 nameHash = HashOf(_name);
        JAM_ASSERT(!g_componentInfoMap.contains(nameHash), "Component '{}' is already registered", _name);
        g_componentInfoMap[nameHash]       = info;
        g_typeIDToNameHashT[info.typeHash] = nameHash;
    }

    void Create(const std::string_view _name, const Entity& _entity)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.createFn, "Component '{}' does not have a create function", _name);
        info.createFn(_entity);
    }

    void CreateByCopy(const std::string_view _name, const Entity& _entity, const std::any& _component)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.createByCopyFn, "Component '{}' does not have a copy constructor function", _name);
        info.createByCopyFn(_entity, _component);
    }

    void CreateOrReplace(std::string_view _name, const Entity& _entity)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.createOrReplaceFn, "Component '{}' does not have a create or replace function", _name);
        info.createOrReplaceFn(_entity);
    }

    void CreateOrReplaceByCopy(std::string_view _name, const Entity& _entity, const std::any& _component)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.createOrReplaceByCopyFn, "Component '{}' does not have a copy constructor for create or replace function", _name);
        info.createOrReplaceByCopyFn(_entity, _component);
    }

    void Remove(const std::string_view _name, const Entity& _entity)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.removeFn, "Component '{}' does not have a remove function", _name);
        info.removeFn(_entity);
    }

    std::any Get(const std::string_view _name, const Entity& _entity)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.getFn, "Component '{}' does not have a get function", _name);
        return info.getFn(_entity);
    }

    bool Has(const std::string_view _name, const Entity& _entity)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.hasFn, "Component '{}' does not have a has function", _name);
        return info.hasFn(_entity);
    }

    Json Serialize(std::string_view _name, const std::any& _component, const Scene* _pScene, const Entity& _entity)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.serializeFn, "Component '{}' does not have a serialize function", _name);
        return info.serializeFn(_component, _pScene, _entity);
    }

    void Deserialize(std::string_view _name, std::any& _component, const Json& _json, const Scene* _pScene, const Entity& _entity)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.deserializeFn, "Component '{}' does not have a deserialize function", _name);
        info.deserializeFn(_component, _json, _pScene, _entity);
    }

    void DrawEditor(std::string_view _name, std::any& _component, const Scene* _pScene, const Entity& _entity)
    {
        const UInt32         hash = HashOf(_name);
        const ComponentInfo& info = g_componentInfoMap.at(hash);
        JAM_ASSERT(info.drawEditorFn, "Component '{}' does not have a draw editor function", _name);
        info.drawEditorFn(_component, _pScene, _entity);
    }

    void Create(const UInt32 _typeID, const Entity& _entity)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.createFn, "Component with type hash {} does not have a create function", _typeID);
        info.createFn(_entity);
    }

    void CreateByCopy(const UInt32 _typeID, const Entity& _entity, const std::any& _component)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.createByCopyFn, "Component with type hash {} does not have a copy constructor function", _typeID);
        info.createByCopyFn(_entity, _component);
    }

    void CreateOrReplace(UInt32 _typeID, const Entity& _entity)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.createOrReplaceFn, "Component with type hash {} does not have a create or replace function", _typeID);
        info.createOrReplaceFn(_entity);
    }

    void CreateOrReplaceByCopy(UInt32 _typeID, const Entity& _entity, const std::any& _component)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.createOrReplaceByCopyFn, "Component with type hash {} does not have a copy constructor for create or replace function", _typeID);
        info.createOrReplaceByCopyFn(_entity, _component);
    }

    void Remove(const UInt32 _typeID, const Entity& _entity)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.removeFn, "Component with type hash {} does not have a remove function", _typeID);
        info.removeFn(_entity);
    }

    std::any Get(const UInt32 _typeID, const Entity& _entity)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.getFn, "Component with type hash {} does not have a get function", _typeID);
        return info.getFn(_entity);
    }

    bool Has(const UInt32 _typeID, const Entity& _entity)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.hasFn, "Component with type hash {} does not have a has function", _typeID);
        return info.hasFn(_entity);
    }

    Json Serialize(UInt32 _typeID, const std::any& _component, const Scene* _pScene, const Entity& _entity)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.serializeFn, "Component with type hash {} does not have a serialize function", _typeID);
        return info.serializeFn(_component, _pScene, _entity);
    }

    void Deserialize(UInt32 _typeID, std::any& _component, const Json& _json, const Scene* _pScene, const Entity& _entity)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.deserializeFn, "Component with type hash {} does not have a deserialize function", _typeID);
        info.deserializeFn(_component, _json, _pScene, _entity);
    }

    void DrawEditor(UInt32 _typeID, std::any& _component, const Scene* _pScene, const Entity& _entity)
    {
        const UInt32         nameHash = g_typeIDToNameHashT.at(_typeID);
        const ComponentInfo& info     = g_componentInfoMap.at(nameHash);
        JAM_ASSERT(info.drawEditorFn, "Component with type hash {} does not have a draw editor function", _typeID);
        info.drawEditorFn(_component, _pScene, _entity);
    }

    const HashMap<UInt32, ComponentInfo>& GetAllComponentInfos()
    {
        return g_componentInfoMap;
    }
}   // namespace component_db
}   // namespace jam