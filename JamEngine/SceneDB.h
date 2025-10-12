#pragma once

// clang-format off
namespace jam { class Scene; }
// clang-format on

namespace jam
{

struct SceneInfo
{
    std::string                             name     = {};
    std::function<std::unique_ptr<Scene>()> createFn = nullptr;
};

namespace scene_db
{
    namespace details
    {
        template<typename T>
        std::unique_ptr<Scene> CreateBehavior()
        {
            return std::make_unique<T>();
        }
    }   // namespace details

    void Reflect(const SceneInfo& _info);

    template<typename T>
    void Reflect()
    {
        SceneInfo info;
        info.name     = NameOf<T>();
        info.createFn = details::CreateBehavior<T>;
        Reflect(info);
    }

    JAM_NODISCARD std::unique_ptr<Scene> Create(std::string_view _name);
    JAM_NODISCARD const HashMap<UInt32, SceneInfo>& GetSceneInfos();

}   // namespace scene_db
}   // namespace jam
