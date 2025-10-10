#include "pch.h"

#include "ScriptDB.h"
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

static HashMap<UInt32, ScriptInfo> g_scriptInfoMap;

////////////////////////////////////////////////////////////

namespace jam::script_db
{

void Reflect(const ScriptInfo& info)
{
    const UInt32 nameHash = HashOf(info.name);
    JAM_ASSERT(!g_scriptInfoMap.contains(nameHash), "Script '{}' is already registered", info.name);
    g_scriptInfoMap[nameHash] = info;
}

std::shared_ptr<Script> Create(std::string_view _name, const Entity& _entity)
{
    UInt32 hash = HashOf(_name);
    JAM_ASSERT(g_scriptInfoMap.contains(hash), "Script '{}' is not registered", _name);
    return g_scriptInfoMap.at(hash).createFn(_entity);
}

const HashMap<unsigned, ScriptInfo>& GetAllScriptInfos()
{
    return g_scriptInfoMap;
}

}   // namespace jam::script_db