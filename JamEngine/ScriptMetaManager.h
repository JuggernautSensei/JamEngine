#pragma once
#include "Entity.h"

namespace jam
{

class Script;
using CreateScriptCallback = std::function<Scope<Script>(const Entity&)>;

struct ScriptMeta
{
    std::string_view     scriptName;
    CreateScriptCallback createCallback;
};

class ScriptMetaManager
{
public:
    using MetaContainer = std::unordered_map<std::string_view, ScriptMeta>;

    ScriptMetaManager() = delete;

    template<typename T>
    static void           RegisterScript();
    static void           RegisterScript(const ScriptMeta& _meta);
    NODISCARD static bool IsRegistered(std::string_view _scriptName);

    NODISCARD static Scope<Script> CreateScript(std::string_view _scriptName, const Entity& _owner);
    NODISCARD static const MetaContainer&    GetContainer();
};

template<typename T>
void ScriptMetaManager::RegisterScript()
{
    static_assert(std::is_base_of_v<Script, T>, "T must be derived from Script");
    ScriptMeta meta;
    meta.scriptName     = NameOf<T>();
    meta.createCallback = [](Entity _owner)
    {
        return CreateScope<T>(_owner);
    };
    RegisterScript(meta);
}

}   // namespace jam
