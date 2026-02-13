#include "pch.h"
#include "ReflectionDB.h"

namespace Jam
{

ReflectionDB& ReflectionDB::GetInstance()
{
    if (!s_pInstance)
    {
        s_pInstance = new ReflectionDB;
    }

    return *s_pInstance;
}

bool ReflectionDB::IsComponentReflectedByEnttHash(
    const UInt32 _enttHash) const
{
    return m_enttHashToTypeHashMap.contains(_enttHash);
}

bool ReflectionDB::IsComponentReflected(
    const UInt32 _typeHash) const
{
    return m_componentMetadataContainer.contains(_typeHash);
}

bool ReflectionDB::IsSceneReflected(
    const UInt32 _typeHash) const
{
    return m_sceneMetadataContainer.contains(_typeHash);
}

bool ReflectionDB::IsComponentReflected(
    const StringView _typeName) const
{
    return m_componentMetadataContainer.contains(_typeName.Hash32());
}

bool ReflectionDB::IsSceneReflected(
    const StringView _typeName) const
{
    return m_sceneMetadataContainer.contains(_typeName.Hash32());
}

const ComponentMetadata& ReflectionDB::GetComponentMetadataByEnttHash(
    const UInt32 _enttHash) const
{
    const UInt32 typeHash = m_enttHashToTypeHashMap.at(_enttHash);
    return m_componentMetadataContainer.at(typeHash);
}

const ComponentMetadata& ReflectionDB::GetComponentMetadata(const UInt32 _typeHash) const
{
    return m_componentMetadataContainer.at(_typeHash);
}

const SceneMetadata& ReflectionDB::GetSceneMetadata(const UInt32 _typeHash) const
{
    return m_sceneMetadataContainer.at(_typeHash);
}

const ComponentMetadata& ReflectionDB::GetComponentMetadata(const StringView _typeName) const
{
    const UInt32 key = _typeName.Hash32();
    return m_componentMetadataContainer.at(key);
}

const SceneMetadata& ReflectionDB::GetSceneMetadata(const StringView _typeName) const
{
    const UInt32 key = _typeName.Hash32();
    return m_sceneMetadataContainer.at(key);
}

const HashMap<UInt32, ComponentMetadata>& ReflectionDB::GetComponentMetadataContainer() const
{
    return m_componentMetadataContainer;
}

const HashMap<UInt32, SceneMetadata>& ReflectionDB::GetSceneMetadataContainer() const
{
    return m_sceneMetadataContainer;
}

void ReflectionDB::DestroyInstance()
{
    delete s_pInstance;
}

}   // namespace Jam