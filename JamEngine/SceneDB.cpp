#include "pch.h"

#include "Scene.h"
#include "SceneDB.h"

namespace jam
{

SceneDB* SceneDB::s_instance = nullptr;

SceneDB& SceneDB::GetInstance()
{
    if (!s_instance)
    {
        s_instance = JAM_NEW(SceneDB);
    }

    return *s_instance;
}

void SceneDB::DestroyInstance()
{
    if (s_instance)
    {
        JAM_DELETE(s_instance);
    }
}

void SceneDB::Reflect(
    const SceneInfo& _info)
{
    const UInt32 hash = HashOf(_info.name);
    JAM_ASSERT(!m_infos.contains(hash), "SceneBehavior with name '{}' already registered", _info.name);
    m_infos.emplace(hash, _info);
}

bool SceneDB::IsReflected(
    const std::string_view _name) const
{
    const UInt32 hash = HashOf(_name);
    return m_infos.contains(hash);
}

Ref<Scene> SceneDB::Create(
    std::string_view _name) const
{
    const UInt32 hash = HashOf(_name);
    JAM_ASSERT(m_infos.contains(hash), "No SceneBehavior registered with name '{}'", _name);
    return m_infos.at(hash).createFn();
}

const HashMap<UInt32, SceneInfo>& SceneDB::GetContainer() const
{
    return m_infos;
}

}   // namespace jam