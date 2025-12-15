#include "pch.h"

#include "ComponentDB.h"
#include "StringUtilities.h"

namespace jam
{

ComponentDB* ComponentDB::s_instance = nullptr;

ComponentDB& ComponentDB::GetInstance()
{
    if (!s_instance)
    {
        s_instance = JAM_NEW(ComponentDB);
    }

    return *s_instance;
}

void ComponentDB::DestroyInstance()
{
    if (s_instance)
    {
        JAM_DELETE(s_instance);
    }
}

void ComponentDB::Reflect(const ComponentInfo& info)
{
    const UInt32 hash = HashOf(info.name);
    JAM_ASSERT(!m_infos.contains(hash), "Component '{}' is already registered", info.name);
    m_infos[hash]                    = info;
    m_typeToNameContainer[info.hash] = hash;
}

void ComponentDB::Create(const std::string_view _name, const Entity _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.createFn, "Component '{}' does not have a create function", _name);
    info.createFn(_entity);
}

void ComponentDB::CreateByCopy(const std::string_view _name, const Entity _entity, const void* _pComponent) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.createByCopyFn, "Component '{}' does not have a copy constructor function", _name);
    info.createByCopyFn(_entity, _pComponent);
}

void ComponentDB::Remove(const std::string_view _name, const Entity _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.removeFn, "Component '{}' does not have a remove function", _name);
    info.removeFn(_entity);
}

void* ComponentDB::Get(const std::string_view _name, const Entity _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.getFn, "Component '{}' does not have a get function", _name);
    return info.getFn(_entity);
}

bool ComponentDB::Has(const std::string_view _name, const Entity _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.hasFn, "Component '{}' does not have a has function", _name);
    return info.hasFn(_entity);
}

Json ComponentDB::Serialize(const std::string_view _name, const void* _pComponent, const Ref<Scene>& _pScene, const Entity _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.serializeFn, "Component '{}' does not have a serialize function", _name);
    return info.serializeFn(_pComponent, _pScene, _entity);
}

void ComponentDB::Deserialize(const std::string_view _name, void* _pComponent, const Json& _json, Ref<Scene>& _pScene, const Entity _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.deserializeFn, "Component '{}' does not have a deserialize function", _name);
    info.deserializeFn(_pComponent, _json, _pScene, _entity);
}

void ComponentDB::DrawEditor(const std::string_view _name, void* _pComponent, Ref<Scene>& _pScene, const Entity _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.drawEditorFn, "Component '{}' does not have a draw editor function", _name);
    info.drawEditorFn(_pComponent, _pScene, _entity);
}

bool ComponentDB::IsSerializable(const std::string_view _name) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    return !!info.serializeFn;
}

bool ComponentDB::IsEditable(const std::string_view _name) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    return !!info.drawEditorFn;
}

std::string_view ComponentDB::GetDisplayName(const std::string_view _name) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    return info.displayName;
}

bool ComponentDB::IsReflected(const std::string_view _name) const
{
    const UInt32 hash = HashOf(_name);
    return m_infos.contains(hash);
}

void ComponentDB::Create(const UInt32 _typeID, const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.createFn, "Component with type hash {} does not have a create function", _typeID);
    info.createFn(_entity);
}

void ComponentDB::CreateByCopy(const UInt32 _typeID, const Entity _entity, const void* _pComponent) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.createByCopyFn, "Component with type hash {} does not have a copy constructor function", _typeID);
    info.createByCopyFn(_entity, _pComponent);
}

void ComponentDB::Remove(const UInt32 _typeID, const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.removeFn, "Component with type hash {} does not have a remove function", _typeID);
    info.removeFn(_entity);
}

void* ComponentDB::Get(const UInt32 _typeID, const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.getFn, "Component with type hash {} does not have a get function", _typeID);
    return info.getFn(_entity);
}

bool ComponentDB::Has(const UInt32 _typeID, const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.hasFn, "Component with type hash {} does not have a has function", _typeID);
    return info.hasFn(_entity);
}

Json ComponentDB::Serialize(const UInt32 _typeID, const void* _pComponent, const Ref<Scene>& _pScene, const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.serializeFn, "Component with type hash {} does not have a serialize function", _typeID);
    return info.serializeFn(_pComponent, _pScene, _entity);
}

void ComponentDB::Deserialize(const UInt32 _typeID, void* _pComponent, const Json& _json, Ref<Scene>& _pScene, const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.deserializeFn, "Component with type hash {} does not have a deserialize function", _typeID);
    info.deserializeFn(_pComponent, _json, _pScene, _entity);
}

void ComponentDB::DrawEditor(const UInt32 _typeID, void* _pComponent, Ref<Scene>& _pScene, const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.drawEditorFn, "Component with type hash {} does not have a draw editor function", _typeID);
    info.drawEditorFn(_pComponent, _pScene, _entity);
}

bool ComponentDB::IsSerializable(const UInt32 _typeID) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    return !!info.serializeFn;
}

bool ComponentDB::IsEditable(const UInt32 _typeID) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    return !!info.drawEditorFn;
}

std::string_view ComponentDB::GetDisplayName(const UInt32 _typeID) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    return info.displayName;
}

std::string_view ComponentDB::GetComponentName(const UInt32 _typeID) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeID);
    const ComponentInfo& info = m_infos.at(hash);
    return info.name;
}

bool ComponentDB::IsReflected(const UInt32 _typeID) const
{
    return m_typeToNameContainer.contains(_typeID);
}

const ComponentDB::ContainerT& ComponentDB::GetContainer() const
{
    return m_infos;
}

}   // namespace jam
