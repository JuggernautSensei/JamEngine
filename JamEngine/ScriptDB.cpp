#include "pch.h"

#include "ScriptDB.h"

namespace jam::script_db
{

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

static HashMap<UInt32, ScriptInfo>& GetScriptInfoMap()
{
    static HashMap<UInt32, ScriptInfo> map;
    return map;
}

////////////////////////////////////////////////////////////

void Reflect(const ScriptInfo& info)
{
    const UInt32 nameHash = HashOf(info.name);
    auto&        map      = GetScriptInfoMap();
    JAM_ASSERT(!map.contains(nameHash), "Script with name '{}' is already registered", info.name);
    map.emplace(nameHash, info);
}

std::shared_ptr<Script> Create(const std::string_view _name, const Entity _entity)
{
    const UInt32 nameHash = HashOf(_name);
    auto&        map      = GetScriptInfoMap();
    JAM_ASSERT(map.contains(nameHash), "No Script registered with name '{}'", _name);
    return map[nameHash].createFn(_entity);
}

const HashMap<unsigned, ScriptInfo>& GetScriptInfos()
{
    auto& map = GetScriptInfoMap();
    return map;
}

}   // namespace jam::script_db