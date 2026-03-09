#pragma once

namespace Jam
{

class Scene;

namespace Details
{
    template<typename T>
    Ref<Scene> CreateScene()
    {
        return MakeRef<T>();
    }

}   // namespace Details

struct SceneReflectionData
{
    StringView             typeName = {};
    UInt32                 typeHash = 0;
    Delegate<Ref<Scene>()> Create   = nullptr;
};

class SceneDatabase
{
public:
    [[nodiscard]] static SceneDatabase& GetInstance();

    template<typename T>
    void Reflect()
    {
        SceneReflectionData data = {};
        data.typeName            = NameOf<T>();
        data.typeHash            = HashOf<T>();
        data.Create              = Bind<Details::CreateScene<T>>();
        const UInt32 typeHash    = data.typeHash;
        JAM_ASSERT(data.typeName.Hash32() == typeHash, "Type typeName hash does not match type hash for scene '{}'", data.typeName);
        JAM_ASSERT(!m_sceneMetadataContainer.contains(typeHash), "Scene with typeName '{}' is already registered", data.typeName);
        m_sceneMetadataContainer.emplace(typeHash, data);
    }

    [[nodiscard]] bool IsSceneReflected(UInt32 _typeHash) const;
    [[nodiscard]] bool IsSceneReflected(StringView _typeName) const;

    [[nodiscard]] const SceneReflectionData&                     GetSceneReflectionData(UInt32 _typeHash) const;
    [[nodiscard]] const SceneReflectionData&                     GetSceneReflectionData(StringView _typeName) const;
    [[nodiscard]] const CASHashMap<UInt32, SceneReflectionData>& GetSceneReflectionDataContainer() const;

private:
    static void DestroyInstance();

    CASHashMap<UInt32, SceneReflectionData> m_sceneMetadataContainer = {};

    inline static SceneDatabase* s_pInstance = nullptr;
    friend class Application;
};

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

#define JAM_SCENE_DB Jam::SceneDatabase::GetInstance()

#define JAM_SCENE_BODY(TScene)                                                      \
public:                                                                             \
    static constexpr StringView k_typeName = Jam::NameOf<TScene>();                 \
    static constexpr UInt32     k_typeHash = Jam::HashOf<TScene>();                 \
    [[nodiscard]] StringView    GetTypeName() const override { return k_typeName; } \
    [[nodiscard]] UInt32        GetTypeHash() const override { return k_typeHash; } \
                                                                                    \
private:                                                                            \
    inline static auto JAM_SCENE_RELFECTED = Jam::Details::SceneReflector<TScene> {}
