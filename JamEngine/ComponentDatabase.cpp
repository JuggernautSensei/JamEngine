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
    return m_componentMetadataContainer.contains(_typeName.Hash32());
}

bool ComponentDatabase::IsComponentReflected(
    const UInt32 _typeHash) const
{
    return m_componentMetadataContainer.contains(_typeHash);
}

bool ComponentDatabase::IsComponentReflectedByEnttHash(
    const UInt32 _enttHash) const
{
    return m_enttHashToTypeHashMap.contains(_enttHash);
}

const ComponentReflectionData& ComponentDatabase::GetComponentReflectionByEnttHash(
    const UInt32 _enttHash) const
{
    const UInt32 typeHash = m_enttHashToTypeHashMap.at(_enttHash);
    return m_componentMetadataContainer.at(typeHash);
}

const ComponentReflectionData& ComponentDatabase::GetComponentReflection(
    const UInt32 _typeHash) const
{
    return m_componentMetadataContainer.at(_typeHash);
}

const ComponentReflectionData& ComponentDatabase::GetComponentReflection(
    const StringView _typeName) const
{
    const UInt32 typeHash = _typeName.Hash32();
    return m_componentMetadataContainer.at(typeHash);
}

const HashMap<UInt32, ComponentReflectionData>& ComponentDatabase::GetComponentReflectionContainer() const
{
    return m_componentMetadataContainer;
}

void ComponentDatabase::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

}   // namespace Jam