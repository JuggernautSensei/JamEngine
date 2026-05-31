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

void SceneDatabase::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

bool SceneDatabase::IsSceneReflected(
    const UInt64 _typeHash) const
{
    return m_sceneRflData.contains(_typeHash);
}

bool SceneDatabase::IsSceneReflected(
    const StringView _name) const
{
    return m_sceneRflData.contains(_name.Hash());
}

const SceneReflectionData& SceneDatabase::GetSceneReflectionData(
    const UInt64 _typeHash) const
{
    return m_sceneRflData.at(_typeHash);
}

const SceneReflectionData& SceneDatabase::GetSceneReflectionData(
    const StringView _name) const
{
    return m_sceneRflData.at(_name.Hash());
}

}   // namespace Jam