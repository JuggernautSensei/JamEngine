#pragma once
#include "Entity.h"

namespace jam
{

class Script;
using CreateScriptCallback = std::function<std::unique_ptr<Script>(const Entity&)>;

struct ScriptMeta
{
    std::string_view     scriptName;
    CreateScriptCallback createCallback;
};

class ScriptMetaManager
{
public:
    ScriptMetaManager() = delete;

    template<typename T>
    static void           RegisterScript();
    static void           RegisterScript(const ScriptMeta& _meta);
    NODISCARD static bool IsRegistered(std::string_view _scriptName);

    NODISCARD static std::unique_ptr<Script> CreateScript(std::string_view _scriptName, const Entity& _owner);
    NODISCARD std::ranges::ref_view<std::unordered_map<std::string_view, ScriptMeta>> GetContainer() const;
};

template<typename T>
void ScriptMetaManager::RegisterScript()
{
    static_assert(std::is_base_of_v<Script, T>, "T must be derived from Script");
    ScriptMeta meta;
    meta.scriptName     = NameOf<T>();
    meta.createCallback = [](Entity _owner)
    {
        return std::make_unique<T>(_owner);
    };
    RegisterScript(meta);
}

}   // namespace jam
