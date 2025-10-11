#include "pch.h"

#include "SceneDB.h"
#include "Scene.h"
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

static HashMap<UInt32, SceneInfo> g_sceneBehaviorInfoMap;

////////////////////////////////////////////////////////////

namespace jam::scene_db
{

void Reflect(const SceneInfo& _info)
{
    UInt32 hash = HashOf(_info.name);
    JAM_ASSERT(!g_sceneBehaviorInfoMap.contains(hash), "SceneBehavior with name '{}' already registered", _info.name);
    g_sceneBehaviorInfoMap.emplace(hash, _info);
}

std::unique_ptr<Scene> Create(std::string_view _name)
{
    UInt32     hash = HashOf(_name);
    const auto it   = g_sceneBehaviorInfoMap.find(hash);
    JAM_ASSERT(it != g_sceneBehaviorInfoMap.end(), "No SceneBehavior registered with name '{}'", _name);
    return it->second.createFn();
}

const HashMap<UInt32, SceneInfo>& GetAllSceneBehaviorInfos()
{
    return g_sceneBehaviorInfoMap;
}

}   // namespace jam::scene_db