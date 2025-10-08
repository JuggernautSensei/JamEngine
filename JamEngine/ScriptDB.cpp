#include "pch.h"

#include "Script.h"
#include "ScriptDB.h"
#include "StringUtilities.h"
using namespace jam;

namespace
{

HashMap<UInt32, ScriptInfo> g_scriptInfoMap;

}

namespace jam
{

namespace script_db
{

    void RefletScript(const std::string_view _name, const ScriptInfo& _info)
    {
        const UInt32 hash = HashOf(_name);
        JAM_ASSERT(g_scriptInfoMap.contains(hash) == false, "Script '{}' is already registered", _name);
        g_scriptInfoMap[hash] = _info;
    }

    std::unique_ptr<Script> Create(const std::string_view _name, const Entity& _entity)
    {
        const UInt32      hash = HashOf(_name);
        const ScriptInfo& info = g_scriptInfoMap.at(hash);
        return info.createFunc(_entity);
    }

    const HashMap<UInt32, ScriptInfo>& GetAllScripts()
    {
        return g_scriptInfoMap;
    }

}   // namespace script_db

}   // namespace jam