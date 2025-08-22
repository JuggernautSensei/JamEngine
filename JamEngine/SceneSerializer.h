#pragma once

namespace jam
{
class AssetManager;
class Scene;

class SceneSerializer
{
public:
    bool LoadFromFile(const fs::path& _filePath);
    bool SaveFromFile(const fs::path& _filePath) const;

    void Serialize(Scene* _pScene);
    void Deserialize(Scene* _pScene);

    void Clear();

private:
    NODISCARD Json SerializeAssetManager(const AssetManager& _assetMgr) const;
    NODISCARD Json SerializeEntity(Scene* _pScene) const;
    NODISCARD Json SerializeComponent(Scene* _scene) const;

    NODISCARD void DeserializeAssetManager(const Json& _json, AssetManager* _pAssetMgr) const;
    NODISCARD void DeserializeEntity(const Json& _json, Scene* _pScene) const;
    NODISCARD void DeserializeComponent(const Json& _json, Scene* _pScene) const;

    NODISCARD bool IsValidSceneExtension(const fs::path& _filePath) const;
    NODISCARD bool IsValidJson(const Json& _json) const;

    Json m_json;
};

}   // namespace jam