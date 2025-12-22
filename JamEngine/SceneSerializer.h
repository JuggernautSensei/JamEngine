#pragma once
#include "Environment.h"
#include "JsonWriter.h"
#include "UID.h"

namespace jam
{

class Scene;

class SceneSerializer
{
public:
    bool SaveJsonFile(const fs::path& _path) const;
    bool SaveBsonFile(const fs::path& _path) const;
    void Serialize(const Ref<Scene>& _pScene);

    [[nodiscard]] const Json& GetJson() const;

private:
    Json m_json = {};
};

}   // namespace jam
