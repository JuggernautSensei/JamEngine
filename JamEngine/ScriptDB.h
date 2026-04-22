#pragma once
#include "Entity.h"

// clang-format off
namespace jam { class Script; }
// clang-format on

namespace jam
{

template<typename T>
concept IsScript = std::is_base_of_v<Script, T>;

struct ScriptInfo
{
    std::string name = {};

    std::function<std::unique_ptr<Script>(const Entity&)> createFunc = nullptr;
};

namespace script_db
{
    namespace details
    {
        template<IsScript T>
        std::unique_ptr<Script> CreateScript(const Entity& _entity)
        {
            return std::make_unique<T>(_entity);
        }
    }   // namespace details

    void Reflect(std::string_view _name, const ScriptInfo& _info);

    template<IsScript T>
    void Reflect(const std::string_view _name)
    {
        ScriptInfo info;
        info.name       = std::string { _name };
        info.createFunc = details::CreateScript<T>;
        Reflect(_name, info);
    }

    JAM_NODISCARD std::unique_ptr<Script> Create(std::string_view _name, const Entity& _entity);
    JAM_NODISCARD const HashMap<UInt32, ScriptInfo>& GetAllScripts();

}   // namespace script_db
}   // namespace jam
