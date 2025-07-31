#include "pch.h"

#include "ScriptMetaManager.h"

#include "Entity.h"

namespace
{

NODISCARD auto& GetScriptMetaMap()
{
    static std::unordered_map<std::string_view, jam::ScriptMeta> s_scriptMetaMap;
    return s_scriptMetaMap;
}

}   // namespace

namespace jam
{

void ScriptMetaManager::RegisterScript(const ScriptMeta& _meta)
{
    auto& map        = GetScriptMetaMap();
    auto [_, result] = map.emplace(_meta.scriptName, _meta);
    JAM_ASSERT(result, "Script with name '{}' already registered", _meta.scriptName);
}

bool ScriptMetaManager::IsRegistered(const std::string_view _scriptName)
{
    auto& map = GetScriptMetaMap();
    return map.contains(_scriptName);
}

std::unique_ptr<Script> ScriptMetaManager::CreateScript(std::string_view _scriptName, const Entity _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is invalid");

    auto& map = GetScriptMetaMap();
    auto  it  = map.find(_scriptName);
    JAM_ASSERT(it != map.end(), "Script with name '{}' not found", _scriptName);

    const ScriptMeta& meta = it->second;
    JAM_ASSERT(meta.createCallback, "Create callback for script '{}' is not set", _scriptName);

    return meta.createCallback(_owner);
}

std::ranges::ref_view<std::unordered_map<std::string_view, ScriptMeta>> ScriptMetaManager::GetScriptMetaView() const
{
    auto& map = GetScriptMetaMap();
    return std::views::all(map);
}

}   // namespace jam