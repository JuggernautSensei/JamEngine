#pragma once
#include "Entity.h"

// clang-format off
namespace jam { class Script; }
// clang-format on

namespace jam
{

struct ScriptInfo
{
    std::string                                    name     = {};
    std::function<std::shared_ptr<Script>(Entity)> createFn = nullptr;
};

namespace details
{
    template<typename T>
    std::shared_ptr<Script> CreateScript(Entity e)
    {
        return std::make_shared<T>(e);
    }
}   // namespace details

class ScriptDB
{
public:
    static void Reflect(const ScriptInfo& info);

    template<typename T>
    static void Reflect()
    {
        ScriptInfo info;
        info.name     = NameOf<T>();
        info.createFn = details::CreateScript<T>;
        Reflect(info);
    }

    JAM_NODISCARD static std::shared_ptr<Script>            Create(std::string_view _name, Entity _entity);
    JAM_NODISCARD static const HashMap<UInt32, ScriptInfo>& GetScriptInfos();

};   // namespace script_db

}   // namespace jam