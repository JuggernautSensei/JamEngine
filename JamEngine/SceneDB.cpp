#include "pch.h"

#include "Scene.h"
#include "SceneDB.h"


namespace jam
{

JAM_NODISCARD static HashMap<UInt32, SceneInfo>& GetSceneInfoMap()
{
    static HashMap<UInt32, SceneInfo> map;
    return map;
}

}

namespace jam
{

void SceneDB::Reflect(const SceneInfo& _info)
{
    UInt32 hash = HashOf(_info.name);
    auto&  map  = GetSceneInfoMap();
    JAM_ASSERT(!map.contains(hash), "SceneBehavior with name '{}' already registered", _info.name);
    map.emplace(hash, _info);
}

std::shared_ptr<Scene> SceneDB::Create(std::string_view _name)
{
    UInt32 hash = HashOf(_name);
    auto&  map  = GetSceneInfoMap();
    JAM_ASSERT(map.contains(hash), "No SceneBehavior registered with name '{}'", _name);
    return map[hash].createFn();
}

const HashMap<UInt32, SceneInfo>& SceneDB::GetSceneInfos()
{
    auto& map = GetSceneInfoMap();
    return map;
}

}   // namespace jam::scene_db