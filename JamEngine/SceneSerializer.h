#pragma once
// clang-format off
namespace jam { class Scene; }
// clang-format on

namespace jam
{

class SceneSerializer
{
public:
    eErrorCode SaveToFile(const fs::path& _filePath) const;
    eErrorCode LoadFromFile(const fs::path& _filePath);

    void          Serialize(const Scene* _pScene);
    JAM_NODISCARD Result<std::unique_ptr<Scene>> Deserialize();

private:
    Json m_json;
};

}   // namespace jam
