#include "pch.h"
#include "SceneDatabase.h"

namespace Jam
{

SceneDatabase& SceneDatabase::GetInstance()
{
    if (!s_pInstance)
    {
        s_pInstance = new SceneDatabase();
    }

    return *s_pInstance;
}

bool SceneDatabase::IsSceneReflected(
    const UInt32 _typeHash) const
{
    return m_sceneMetadataContainer.contains(_typeHash);
}

bool SceneDatabase::IsSceneReflected(
    const StringView _typeName) const
{
    return m_sceneMetadataContainer.contains(_typeName.Hash32());
}

const SceneReflectionData& SceneDatabase::GetSceneReflectionData(
    const UInt32 _typeHash) const
{
    return m_sceneMetadataContainer.at(_typeHash);
}

const SceneReflectionData& SceneDatabase::GetSceneReflectionData(
    const StringView _typeName) const
{
    const UInt32 typeHash = _typeName.Hash32();
    return m_sceneMetadataContainer.at(typeHash);
}

const HashMap<UInt32, SceneReflectionData>& SceneDatabase::GetSceneReflectionDataContainer() const
{
    return m_sceneMetadataContainer;
}

void SceneDatabase::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

}   // namespace Jam