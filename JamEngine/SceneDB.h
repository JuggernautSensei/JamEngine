#pragma once

// clang-format off
namespace jam { class Scene; }
// clang-format on

namespace jam
{

struct SceneInfo
{
    std::string                 name     = {};
    std::function<Ref<Scene>()> createFn = nullptr;
};

namespace details
{
    template<typename T>
    Ref<Scene> CreateScene()
    {
        return MakeRef<T>();
    }
}   // namespace details

class SceneDB
{
public:
    using ContainerT = HashMap<UInt32, SceneInfo>;

    [[nodiscard]] static SceneDB& GetInstance();
    static void                   DestroyInstance();

    void Reflect(const SceneInfo& _info);

    template<typename T>
    void Reflect()
    {
        SceneInfo info;
        info.name     = NameOf<T>();
        info.createFn = details::CreateScene<T>;
        Reflect(info);
    }

    [[nodiscard]] Ref<Scene>        Create(std::string_view _name) const;
    [[nodiscard]] const ContainerT& GetContainer() const;

private:
    static SceneDB* s_instance;

    ContainerT m_infos;
};
// namespace scene_db

}   // namespace jam
