#pragma once
#include "Scene.h"

namespace jam
{

class SceneDeserializer
{
public:
    SceneDeserializer() = default;
    explicit SceneDeserializer(bool _bAllowPartialSucceeded = false);

    bool LoadJsonFile(const fs::path& _path);
    bool LoadBsonFile(const fs::path& _path);
    void LoadJson(const Json& _json);

    bool                     Deserialize();
    [[nodiscard]] Ref<Scene> GetScene() const;

private:
    Json       m_json                   = {};
    Ref<Scene> m_pScene                 = nullptr;
    bool       m_bAllowPartialSucceeded = false;
};

}   // namespace jam