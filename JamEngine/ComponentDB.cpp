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
    JAM_DELETE(s_instance);
}

void ComponentDB::Reflect(
    const ComponentInfo& info)
{
    const UInt32 hash = HashOf(info.name);
    JAM_ASSERT(!m_infos.contains(hash), "Component '{}' is already registered", info.name);
    m_infos[hash]                    = info;
    m_typeToNameContainer[info.hash] = hash;
}

void ComponentDB::Create(
    const StringView _name,
    const Entity     _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.createFn, "Component '{}' does not have a create function", _name);
    info.createFn(_entity);
}

void ComponentDB::CreateByCopy(
    const StringView _name,
    const Entity     _entity,
    const void*      _pComponent) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.createByCopyFn, "Component '{}' does not have a copy constructor function", _name);
    info.createByCopyFn(_entity, _pComponent);
}

void ComponentDB::Remove(
    const StringView _name,
    const Entity     _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.removeFn, "Component '{}' does not have a remove function", _name);
    info.removeFn(_entity);
}

void* ComponentDB::Get(
    const StringView _name,
    const Entity     _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.getFn, "Component '{}' does not have a get function", _name);
    return info.getFn(_entity);
}

bool ComponentDB::Has(
    const StringView _name,
    const Entity     _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.hasFn, "Component '{}' does not have a has function", _name);
    return info.hasFn(_entity);
}

void ComponentDB::Serialize(
    StringView        _name,
    const void*       _pComponent,
    const JsonWriter  _writer,
    const Ref<Scene>& _pScene,
    const Entity      _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.serializeFn, "Component '{}' does not have a serialize function", _name);
    info.serializeFn(_pComponent, _writer, _pScene, _entity);
}

void ComponentDB::Deserialize(
    StringView        _name,
    void*             _pComponent,
    JsonReader        _json,
    const Ref<Scene>& _pScene,
    const Entity      _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.deserializeFn, "Component '{}' does not have a deserialize function", _name);
    info.deserializeFn(_pComponent, _json, _pScene, _entity);
}

void ComponentDB::Serialize(
    UInt32            _typeId,
    const void*       _pComponent,
    const JsonWriter  _writer,
    const Ref<Scene>& _pScene,
    const Entity      _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.serializeFn, "Component with type hash {} does not have a serialize function", _typeId);
    info.serializeFn(_pComponent, _writer, _pScene, _entity);
}

void ComponentDB::Deserialize(
    UInt32            _typeId,
    void*             _pComponent,
    JsonReader        _json,
    const Ref<Scene>& _pScene,
    const Entity      _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.deserializeFn, "Component with type hash {} does not have a deserialize function", _typeId);
    info.deserializeFn(_pComponent, _json, _pScene, _entity);
}

void ComponentDB::DrawEditor(
    const StringView _name,
    void*            _pComponent,
    Ref<Scene>&      _pScene,
    const Entity     _entity) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.drawEditorFn, "Component '{}' does not have a draw editor function", _name);
    info.drawEditorFn(_pComponent, _pScene, _entity);
}

bool ComponentDB::IsSerializable(
    const StringView _name) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    return !!info.serializeFn;
}

bool ComponentDB::IsEditable(
    const StringView _name) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    return !!info.drawEditorFn;
}

StringView ComponentDB::GetDisplayName(
    const StringView _name) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infos.at(hash);
    return info.displayName;
}

bool ComponentDB::IsReflected(
    const StringView _name) const
{
    const UInt32 hash = HashOf(_name);
    return m_infos.contains(hash);
}

void ComponentDB::Create(
    const UInt32 _typeId,
    const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.createFn, "Component with type hash {} does not have a create function", _typeId);
    info.createFn(_entity);
}

void ComponentDB::CreateByCopy(
    const UInt32 _typeId,
    const Entity _entity,
    const void*  _pComponent) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.createByCopyFn, "Component with type hash {} does not have a copy constructor function", _typeId);
    info.createByCopyFn(_entity, _pComponent);
}

void ComponentDB::Remove(
    const UInt32 _typeId,
    const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.removeFn, "Component with type hash {} does not have a remove function", _typeId);
    info.removeFn(_entity);
}

void* ComponentDB::Get(
    const UInt32 _typeId,
    const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.getFn, "Component with type hash {} does not have a get function", _typeId);
    return info.getFn(_entity);
}

bool ComponentDB::Has(
    const UInt32 _typeId,
    const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.hasFn, "Component with type hash {} does not have a has function", _typeId);
    return info.hasFn(_entity);
}

void ComponentDB::DrawEditor(
    const UInt32 _typeId,
    void*        _pComponent,
    Ref<Scene>&  _pScene,
    const Entity _entity) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    JAM_ASSERT(info.drawEditorFn, "Component with type hash {} does not have a draw editor function", _typeId);
    info.drawEditorFn(_pComponent, _pScene, _entity);
}

bool ComponentDB::IsSerializable(
    const UInt32 _typeId) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    return !!info.serializeFn;
}

bool ComponentDB::IsEditable(
    const UInt32 _typeId) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    return !!info.drawEditorFn;
}

StringView ComponentDB::GetDisplayName(
    const UInt32 _typeId) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    return info.displayName;
}

StringView ComponentDB::GetComponentName(
    const UInt32 _typeId) const
{
    const UInt32         hash = m_typeToNameContainer.at(_typeId);
    const ComponentInfo& info = m_infos.at(hash);
    return info.name;
}

bool ComponentDB::IsReflected(
    const UInt32 _typeId) const
{
    return m_typeToNameContainer.contains(_typeId);
}

const ComponentDB::ContainerT& ComponentDB::GetContainer() const
{
    return m_infos;
}

}   // namespace jam
