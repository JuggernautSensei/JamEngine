#pragma once
#include "Entity.h"
#include "Script.h"

namespace jam
{

struct ScriptInfo
{
    std::string                        name     = {};
    std::function<Ref<Script>(Entity)> createFn = nullptr;
};

namespace details
{
    template<typename T>
    Ref<Script> CreateScript(Entity e)
    {
        return MakeRef<T>(e);
    }
}   // namespace details

class ScriptDB
{
public:
    using ContainerT = HashMap<UInt32, ScriptInfo>;

    [[nodiscard]] static ScriptDB& GetInstance();
    static void                    DestroyInstance();

    void Reflect(const ScriptInfo& info);

    template<typename T>
    void Reflect()
    {
        ScriptInfo info;
        info.name     = NameOf<T>();
        info.createFn = details::CreateScript<T>;
        Reflect(info);
    }

    [[nodiscard]] Ref<Script>       Create(std::string_view _name, Entity _entity) const;
    [[nodiscard]] const ContainerT& GetContainer() const;

private:
    static ScriptDB* s_pInstance;

    HashMap<UInt32, ScriptInfo> m_infos;

};   // namespace script_db

}   // namespace jam