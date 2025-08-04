#pragma once

namespace jam
{
class Scene;

class SceneSerializer
{
public:
    bool LoadFromFile(const fs::path& _filePath);
    bool SaveFromFile(const fs::path& _filePath) const;

    void Serialize(const Scene* _pScene);
    void Deserialize(Scene* _pScene);

    void Clear();

private:
    Json m_json;
};

}   // namespace jam