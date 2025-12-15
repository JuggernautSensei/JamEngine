#include "pch.h"

#include "SceneSerializer.h"

#include "ComponentDB.h"
#include "Profile.h"
#include "Scene.h"

namespace jam
{
    
bool SceneSerializer::SaveFile(const fs::path& _path) const
{
    JAM_ASSERT(!_path.empty(), "file path is empty");
    JAM_ASSERT(!m_json.empty(), "no data to serialize");
    return SaveBsonFile(_path, m_json);
}

void SceneSerializer::Serialize(const Ref<Scene>& _pScene)
{
    JAM_ASSERT(_pScene, "scene is null");
    JAM_FUNCTION_MSG_PROFILE("scene name: {}", _pScene->GetTypeName());

    m_json.clear();
    m_json["sceneType"] = _pScene->GetTypeName();

    const HashMap<UID, Entity>& entities = _pScene->GetEntities();
    HashMap<UInt32, UID>        enttUidMap;
    enttUidMap.reserve(entities.size());

    // entity list
    {
        std::vector<UID> uids;
        uids.reserve(entities.size());
        for (const auto& [uid, entity]: entities)
        {
            const UInt32 entt = entt::to_integral(entity.GetHandle());
            uids.push_back(uid);
            enttUidMap[entt] = uid;
        }

        m_json["uids"] = std::move(uids);
    }

    // components
    {
        Json         cmpsJson;
        ComponentDB& cmpDb = ComponentDB::GetInstance();

        const entt::registry& reg = _pScene->GetRegistry();
        for (const auto& [typeId, storage]: reg.storage())
        {
            if (!cmpDb.IsReflected(typeId) || !cmpDb.IsSerializable(typeId))
            {
                continue;
            }

            if (storage.empty())
            {
                continue;
            }

            Json cmpJson;
            for (const entt::entity handle: storage)
            {
                const UInt32 entt = entt::to_integral(handle);

                const auto it = enttUidMap.find(entt);
                if (it == enttUidMap.end())
                {
                    continue;
                }

                const UID    uid          = it->second;
                const Entity entity       = entities.at(uid);
                const void*  pCmpValue    = storage.value(handle);
                const Json   cmpValueJson = cmpDb.Serialize(typeId, pCmpValue, _pScene, entity);

                const std::string strUid = std::to_string(uid);
                if (cmpValueJson.is_null() || cmpValueJson.empty())   // empty component return empty or null json
                {
                    cmpJson[strUid] = nullptr;
                }
                else
                {
                    cmpJson[strUid] = cmpValueJson;
                }
            }

            JAM_ASSERT(!cmpJson.empty(), "component json is empty for type ID: {}", typeId);
            const std::string typeName = std::to_string(typeId);
            cmpsJson[typeName]         = std::move(cmpJson);
        }

        m_json["cmps"] = std::move(cmpsJson);
    }
}

const Json& SceneSerializer::GetJson() const
{
    return m_json;
}

}   // namespace jam