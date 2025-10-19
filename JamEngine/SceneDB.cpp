#include "pch.h"

#include "Scene.h"
#include "SceneDB.h"


namespace jam::scene_db
{

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

static HashMap<UInt32, SceneInfo>& GetSceneInfoMap()
{
    static HashMap<UInt32, SceneInfo> map;
    return map;
}

////////////////////////////////////////////////////////////

void Reflect(const SceneInfo& _info)
{
    UInt32 hash = HashOf(_info.name);
    auto&  map  = GetSceneInfoMap();
    JAM_ASSERT(!map.contains(hash), "SceneBehavior with name '{}' already registered", _info.name);
    map.emplace(hash, _info);
}

std::unique_ptr<Scene> Create(std::string_view _name)
{
    UInt32 hash = HashOf(_name);
    auto&  map  = GetSceneInfoMap();
    JAM_ASSERT(map.contains(hash), "No SceneBehavior registered with name '{}'", _name);
    return map[hash].createFn();
}

const HashMap<UInt32, SceneInfo>& GetSceneInfos()
{
    auto& map = GetSceneInfoMap();
    return map;
}

}   // namespace jam::scene_db