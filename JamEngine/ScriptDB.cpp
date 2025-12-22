#include "pch.h"

#include "ScriptDB.h"

namespace jam
{

ScriptDB* ScriptDB::s_pInstance = nullptr;

ScriptDB& ScriptDB::GetInstance()
{
    if (!s_pInstance)
    {
        s_pInstance = JAM_NEW(ScriptDB);
    }

    return *s_pInstance;
}

void ScriptDB::DestroyInstance()
{
    JAM_DELETE(s_pInstance);
}

void ScriptDB::Reflect(const ScriptInfo& info)
{
    const UInt32 hash = HashOf(info.name);
    JAM_ASSERT(!m_infos.contains(hash), "Script with name '{}' is already registered", info.name);
    m_infos.emplace(hash, info);
}

bool ScriptDB::IsReflected(const StringView _name) const
{
    const UInt32 hash = HashOf(_name);
    return m_infos.contains(hash);
}

Ref<Script> ScriptDB::Create(const StringView _name, const Entity _entity) const
{
    const UInt32 hash = HashOf(_name);
    JAM_ASSERT(m_infos.contains(hash), "No Script registered with name '{}'", _name);
    return m_infos.at(hash).createFn(_entity);
}

const ScriptDB::ContainerT& ScriptDB::GetContainer() const
{
    return m_infos;
}

}   // namespace jam