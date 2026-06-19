#pragma once

namespace Jam
{

class Scene;

class SceneDatabase
{

    template<typename T>
        requires std::derived_from<T, Scene>
    [[nodiscard]] Scoped<Scene> MakeScene_() const
    {
        return MakeScoped<T>();
    }

public:
    struct Data
    {
        StringView                name = {};
        UInt64                    hash = 0;
        Delegate<Scoped<Scene>()> Make = nullptr;
    };

    static SceneDatabase& GetInstance();

    SceneDatabase(const SceneDatabase&)            = delete;
    SceneDatabase& operator=(const SceneDatabase&) = delete;
    SceneDatabase(SceneDatabase&&)                 = delete;
    SceneDatabase& operator=(SceneDatabase&&)      = delete;

    template<typename T>
    void Register()
    {
        Data data = {};
        data.name = NameOf<T>();
        data.hash = HashOf<T>();
        data.Make = Bind<&SceneDatabase::MakeScene_<T>>(this);
        JAM_ASSERT(!m_dataMap.contains(data.hash), "Scene with typeName '{}' is already registered", data.name);
        m_dataMap[data.hash] = data;
    }

    [[nodiscard]] bool IsSceneRegistered(UInt64 _typeHash) const;
    [[nodiscard]] bool IsSceneRegistered(StringView _name) const;

    [[nodiscard]] const Data&                       GetSceneData(UInt64 _typeHash) const;
    [[nodiscard]] const Data&                       GetSceneData(StringView _name) const;
    [[nodiscard]] const PreHashedMap<UInt64, Data>& GetSceneDataMap() const;

private:
    SceneDatabase()  = default;
    ~SceneDatabase() = default;

    static void DestroyInstance();

    PreHashedMap<UInt64, Data>   m_dataMap   = {};   // [hash, Data]
    inline static SceneDatabase* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] inline SceneDatabase& GetSceneDatabase()
{
    return SceneDatabase::GetInstance();
}

namespace Details
{
    template<typename T>
    class SceneRegister
    {
    public:
        SceneRegister()
        {
            SceneDatabase& sd = SceneDatabase::GetInstance();
            sd.Register<T>();
        }
    };

}   // namespace Details

}   // namespace Jam

#define JAM_SCENE_BODY(TScene, ...)                                                    \
private:                                                                               \
    inline static Jam::Details::SceneRegister<TScene> JAM_SCENE_REGISTERED = {};       \
                                                                                       \
public:                                                                                \
    [[nodiscard]] StringView GetTypeName() const override { return NameOf<TScene>(); } \
    [[nodiscard]] UInt64     GetTypeHash() const override { return HashOf<TScene>(); } \
                                                                                       \
private:                                                                               \
    static_assert(true)
