#pragma once

// clang-format off
namespace jam { class Script; class Entity; }
// clang-format on

namespace jam
{

struct ScriptInfo
{
    std::string                                           name     = {};
    std::function<std::unique_ptr<Script>(const Entity&)> createFn = nullptr;
};

namespace script_db
{
    namespace details
    {
        template<typename T>
        std::unique_ptr<Script> CreateScript(const Entity& e)
        {
            return std::make_unique<T>(e);
        }
    }   // namespace details

    void Reflect(const ScriptInfo& info);

    template<typename T>
    void Reflect()
    {
        ScriptInfo info;
        info.name     = NameOf<T>();
        info.createFn = details::CreateScript<T>;
        Reflect(info);
    }

    JAM_NODISCARD std::unique_ptr<Script> Create(std::string_view _name, const Entity& _entity);
    JAM_NODISCARD const HashMap<UInt32, ScriptInfo>& GetAllScriptInfos();

}   // namespace script_db

}   // namespace jam