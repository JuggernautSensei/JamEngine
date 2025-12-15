#pragma once
#include "Scene.h"

namespace jam
{

class SceneDeserializer
{
public:
    bool LoadFile(const fs::path& _path);
    void LoadJson(const Json& _json);

    bool                     Deserialize();
    [[nodiscard]] Ref<Scene> GetScene() const;

private:
    Json       m_json   = {};
    Ref<Scene> m_pScene = nullptr;
};

}   // namespace jam