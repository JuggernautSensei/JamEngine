#include "pch.h"
#include "ComponentDatabase.h"

namespace Jam
{

ComponentDatabase& ComponentDatabase::GetInstance()
{
    if (!s_pInstance)
    {
        s_pInstance = new ComponentDatabase();
    }
    return *s_pInstance;
}

bool ComponentDatabase::IsComponentRegistered(
    const StringView _typeName) const
{
    return m_componentDataMap.contains(_typeName.Hash());
}

bool ComponentDatabase::IsComponentRegistered(
    const UInt64 _typeHash) const
{
    return m_componentDataMap.contains(_typeHash);
}

bool ComponentDatabase::IsComponentRegisteredByENTTHash(
    const UInt32 _enttHash) const
{
    return m_enttHashToTypeHash.contains(_enttHash);
}

const ComponentDatabase::Data& ComponentDatabase::GetComponentDataByENTTHash(
    const UInt32 _enttHash) const
{
    const UInt64 typeHash = m_enttHashToTypeHash.at(_enttHash);
    return m_componentDataMap.at(typeHash);
}

const ComponentDatabase::Data& ComponentDatabase::GetComponentData(
    const UInt64 _typeHash) const
{
    return m_componentDataMap.at(_typeHash);
}

const ComponentDatabase::Data& ComponentDatabase::GetComponentData(
    const StringView _typeName) const
{
    const UInt64 typeHash = _typeName.Hash();
    return m_componentDataMap.at(typeHash);
}

const PreHashedMap<UInt64, ComponentDatabase::Data>& ComponentDatabase::GetComponentDataContainer() const
{
    return m_componentDataMap;
}

void ComponentDatabase::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

}   // namespace Jam