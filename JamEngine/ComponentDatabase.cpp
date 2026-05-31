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

bool ComponentDatabase::IsComponentReflected(
    const StringView _typeName) const
{
    return m_componentRflData.contains(_typeName.Hash());
}

bool ComponentDatabase::IsComponentReflected(
    const UInt64 _typeHash) const
{
    return m_componentRflData.contains(_typeHash);
}

bool ComponentDatabase::IsComponentReflectedByEnttHash(
    const UInt32 _enttHash) const
{
    return m_enttHashToTypeHash.contains(_enttHash);
}

const ComponentReflectionData& ComponentDatabase::GetComponentReflectionByEnttHash(
    const UInt32 _enttHash) const
{
    const UInt64 typeHash = m_enttHashToTypeHash.at(_enttHash);
    return m_componentRflData.at(typeHash);
}

const ComponentReflectionData& ComponentDatabase::GetComponentReflection(
    const UInt64 _typeHash) const
{
    return m_componentRflData.at(_typeHash);
}

const ComponentReflectionData& ComponentDatabase::GetComponentReflection(
    const StringView _typeName) const
{
    const UInt64 typeHash = _typeName.Hash();
    return m_componentRflData.at(typeHash);
}

const CASHashMap<UInt64, ComponentReflectionData>& ComponentDatabase::GetComponentReflectionContainer() const
{
    return m_componentRflData;
}

void ComponentDatabase::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

}   // namespace Jam