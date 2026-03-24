#pragma once

namespace Jam
{

class Scene;

namespace Details
{
    template<typename T>
        requires std::derived_from<T, Scene>
    [[nodiscard]] Scoped<Scene> CreateScene()
    {
        return MakeScoped<T>();
    }

}   // namespace Details

struct SceneReflectionData
{
    StringView                typeName = {};
    UInt32                    typeHash = 0;
    Delegate<Scoped<Scene>()> Create   = nullptr;
};

class SceneDatabase
{
public:
    static SceneDatabase& GetInstance();

    SceneDatabase(const SceneDatabase&)            = delete;
    SceneDatabase& operator=(const SceneDatabase&) = delete;
    SceneDatabase(SceneDatabase&&)                 = delete;
    SceneDatabase& operator=(SceneDatabase&&)      = delete;

    template<typename T>
    void Reflect()
    {
        SceneReflectionData data = {};
        data.typeName            = NameOf<T>();
        data.typeHash            = HashOf<T>();
        data.Create              = Bind<Details::CreateScene<T>>();
        JAM_ASSERT(!IsSceneReflected(data.typeHash), "Scene type '{}' is already reflected.", data.typeName);
        JAM_ASSERT(data.typeName.Hash32() == data.typeHash, "Scene type typeName '{}' hash does not match type hash {}. Please check the JAM_SCENE macro usage in the scene class definition.", data.typeName, data.typeHash);
        m_sceneRflData[data.typeHash] = data;
    }

    [[nodiscard]] bool                       IsSceneReflected(UInt32 _typeHash) const;
    [[nodiscard]] bool                       IsSceneReflected(StringView _name) const;
    [[nodiscard]] const SceneReflectionData& GetSceneReflectionData(UInt32 _typeHash) const;
    [[nodiscard]] const SceneReflectionData& GetSceneReflectionData(StringView _name) const;

private:
    SceneDatabase()  = default;
    ~SceneDatabase() = default;

    static void DestroyInstance();

    CASHashMap<UInt32, SceneReflectionData> m_sceneRflData = {};
    inline static SceneDatabase*            s_pInstance    = nullptr;
    friend class Application;
};

[[nodiscard]] inline SceneDatabase& GetSceneDatabase()
{
    return SceneDatabase::GetInstance();
}

namespace Details
{
    template<typename T>
    class SceneReflector
    {
    public:
        SceneReflector()
        {
            SceneDatabase::GetInstance().Reflect<T>();
        }
    };

}   // namespace Details

}   // namespace Jam

#define JAM_SCENE_BODY(TScene, ...)                                                    \
private:                                                                               \
    inline static Jam::Details::SceneReflector<TScene> JAM_SCENE_REFLECTED = {};       \
                                                                                       \
public:                                                                                \
    [[nodiscard]] StringView GetTypeName() const override { return NameOf<TScene>(); } \
    [[nodiscard]] UInt32     GetTypeHash() const override { return HashOf<TScene>(); } \
                                                                                       \
private:                                                                               \
    static_assert(true)
