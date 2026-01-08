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
    const UInt32 hash = HashOf(info.typeName);
    JAM_ASSERT(!m_infoMap.contains(hash), "Component '{}' is already registered", info.typeName);
    m_infoMap[hash]                        = info;
    m_typeToNameMap[info.typeHash] = hash;
}

StringView ComponentDB::GetComponentName(
    const UInt32 _typeId) const
{
    const UInt32         hash = m_typeToNameMap.at(_typeId);
    const ComponentInfo& info = m_infoMap.at(hash);
    return info.typeName;
}

bool ComponentDB::IsReflected(
    const StringView _name) const
{
    const UInt32 hash = HashOf(_name);
    return m_infoMap.contains(hash);
}

bool ComponentDB::IsReflected(
    const UInt32 _typeId) const
{
    return m_typeToNameMap.contains(_typeId);
}

const ComponentDB::ComponentInfoMap& ComponentDB::GetComponentInfoMap() const
{
    return m_infoMap;
}

const ComponentInfo& ComponentDB::GetComponentInfo(
    const StringView _name) const
{
    const UInt32         hash = HashOf(_name);
    const ComponentInfo& info = m_infoMap.at(hash);
    return info;
}

const ComponentInfo& ComponentDB::GetComponentInfo(
    const UInt32 _typeId) const
{
    const UInt32         hash = m_typeToNameMap.at(_typeId);
    const ComponentInfo& info = m_infoMap.at(hash);
    return info;
}

}   // namespace jam
