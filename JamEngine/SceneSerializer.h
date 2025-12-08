#pragma once
#include "Environment.h"

namespace jam
{

class Scene;

class SceneSerializer
{
public:
    bool SaveToFile(const fs::path& _filePath) const;
    void Serialize(const Ref<Scene>& _pScene);

    [[nodiscard]] const Json& GetJson() const;

private:
    Json m_json = {};
};

}   // namespace jam
