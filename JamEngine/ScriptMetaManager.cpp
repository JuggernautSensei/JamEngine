#include "pch.h"

#include "ScriptMetaManager.h"

#include "Entity.h"
#include "Script.h"

namespace
{

using namespace jam;

NODISCARD auto& GetMetaContainer()
{
    static ScriptMetaManager::MetaContainer s_scriptMetaMap;
    return s_scriptMetaMap;
}

}   // namespace

namespace jam
{

void ScriptMetaManager::RegisterScript(const ScriptMeta& _meta)
{
    auto& map        = GetMetaContainer();
    auto [_, result] = map.emplace(_meta.scriptName, _meta);
    JAM_ASSERT(result, "Script with name '{}' already registered", _meta.scriptName);
}

bool ScriptMetaManager::IsRegistered(const std::string_view _scriptName)
{
    auto& map = GetMetaContainer();
    return map.contains(_scriptName);
}

Scope<Script> ScriptMetaManager::CreateScript(std::string_view _scriptName, const Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is invalid");

    auto& map = GetMetaContainer();
    auto  it  = map.find(_scriptName);
    JAM_ASSERT(it != map.end(), "Script with name '{}' not found", _scriptName);

    const ScriptMeta& meta = it->second;
    JAM_ASSERT(meta.createCallback, "Create callback for script '{}' is not set", _scriptName);

    return meta.createCallback(_owner);
}

const ScriptMetaManager::MetaContainer& ScriptMetaManager::GetContainer()
{
    return ::GetMetaContainer();
}

}   // namespace jam