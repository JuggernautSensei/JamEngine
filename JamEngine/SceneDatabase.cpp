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

bool SceneDatabase::IsSceneRegistered(
    const UInt64 _typeHash) const
{
    return m_sceneDataMap.contains(_typeHash);
}

bool SceneDatabase::IsSceneRegistered(
    const StringView _name) const
{
    return m_sceneDataMap.contains(_name.Hash());
}

const SceneDatabase::Data& SceneDatabase::GetSceneData(
    const UInt64 _typeHash) const
{
    return m_sceneDataMap.at(_typeHash);
}

const SceneDatabase::Data& SceneDatabase::GetSceneData(
    const StringView _name) const
{
    return m_sceneDataMap.at(_name.Hash());
}

}   // namespace Jam