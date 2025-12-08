#pragma once
#include "Scene.h"

namespace jam
{

class SceneDeserializer
{
public:
    bool LoadFromFile(const fs::path& _filePath);
    void LoadFromJson(const Json& _json);

    bool                     Deserialize();
    [[nodiscard]] Ref<Scene> GetScene() const;

private:
    Json       m_json   = {};
    Ref<Scene> m_pScene = nullptr;
};

}   // namespace jam