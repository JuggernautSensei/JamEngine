#pragma once

namespace jam
{
class AssetManager;
class Scene;

class SceneSerializer
{
public:
    void           Load(const Json& _json) { m_json = _json; }
    bool           LoadFromFile(const fs::path& _filePath);
    bool           SaveFromFile(const fs::path& _filePath) const;
    JAM_NODISCARD Json GetJson() const { return m_json; }

    void Serialize(Scene* _pScene);
    void Deserialize(Scene* _pScene);

    void Clear();

private:
    JAM_NODISCARD Json SerializeAssetManager(const AssetManager& _assetMgr) const;
    JAM_NODISCARD Json SerializeEntity(const Scene* _pScene) const;
    JAM_NODISCARD Json SerializeComponent(Scene* _scene) const;

    JAM_NODISCARD void DeserializeAssetManager(const Json& _json, AssetManager* _pAssetMgr) const;
    JAM_NODISCARD void DeserializeEntity(const Json& _json, Scene* _pScene) const;
    JAM_NODISCARD void DeserializeComponent(const Json& _json, Scene* _pScene) const;

    JAM_NODISCARD bool IsValidSceneExtension(const fs::path& _filePath) const;
    JAM_NODISCARD bool IsValidJson(const Json& _json) const;

    Json m_json;
};

}   // namespace jam