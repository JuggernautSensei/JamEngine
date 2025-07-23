#include "pch.h"

#include "ScriptMetaManager.h"

#include "Entity.h"

namespace
{

struct ScriptMetaState
{
    std::unordered_map<std::string_view, jam::ScriptMeta> scriptMap;
};

ScriptMetaState g_scriptMetaState;

}   // namespace

namespace jam
{

void ScriptMetaManager::RegisterScript(const ScriptMeta& _meta)
{
    auto [_, result] = g_scriptMetaState.scriptMap.emplace(_meta.scriptName, _meta);
    JAM_ASSERT(result, "Script with name '{}' already registered", _meta.scriptName);
}

std::unique_ptr<Script> ScriptMetaManager::CreateScript(std::string_view _scriptName, const Entity _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is invalid");

    auto it = g_scriptMetaState.scriptMap.find(_scriptName);
    JAM_ASSERT(it != g_scriptMetaState.scriptMap.end(), "Script with name '{}' not found", _scriptName);

    const ScriptMeta& meta = it->second;
    JAM_ASSERT(meta.createCallback, "Create callback for script '{}' is not set", _scriptName);

    return meta.createCallback(_owner);
}

std::ranges::ref_view<std::unordered_map<std::string_view, ScriptMeta>> ScriptMetaManager::GetScriptMetaView() const
{
    return std::views::all(g_scriptMetaState.scriptMap);
}

}   // namespace jam