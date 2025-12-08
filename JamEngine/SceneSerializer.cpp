#include "pch.h"

#include "SceneSerializer.h"

#include "ComponentDB.h"
#include "FileUtilities.h"
#include "ImageTexture.h"
#include "Profile.h"
#include "Scene.h"

#include "JsonUtilities.h"

namespace jam
{

bool SceneSerializer::SaveToFile(const fs::path& _filePath) const
{
    JAM_ASSERT(!_filePath.empty(), "File path is not set");
    JAM_ASSERT(!m_json.empty(), "No data to export");
    return SaveJsonToFile(_filePath, m_json, 0);
}

void SceneSerializer::Serialize(const Ref<Scene>& _pScene)
{
    JAM_ASSERT(_pScene, "Scene is null");
    JAM_FUNCTION_MSG_PROFILE("scene name: {}", _pScene->GetTypeName());

    m_json.clear();
    m_json["sceneType"] = _pScene->GetTypeName();

    const HashMap<UID, Entity>& entities = _pScene->GetEntities();
    HashMap<UInt32, UID>        enttUidMap;   // [entity handle, uid]
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
        Json         json;
        ComponentDB& compDB = ComponentDB::GetInstance();

        const entt::registry& registry = _pScene->GetRegistry();
        for (const auto& [id, storage]: registry.storage())
        {
            if (!compDB.IsSerializable(id))
                continue;

            Json compJson;
            for (entt::entity handle: storage)
            {
                const Int32       entt         = entt::to_integral(handle);
                const UID         uid          = enttUidMap.at(entt);
                const Entity      entity       = entities.at(uid);
                const void*       pCompData    = storage.value(handle);
                const std::string uidStr       = std::to_string(uid);
                const Json        compDataJson = compDB.Serialize(id, pCompData, _pScene, entity);

                if (compDataJson.is_null() || compDataJson.empty())   
                {
                    // emtry struct is null or empty
                    compJson[uidStr] = nullptr;
                }
                else
                {
                    compJson[uidStr] = std::move(compDataJson);
                }
            }

            if (!compJson.empty())
            {
                const std::string key = std::to_string(id);
                json[key]             = std::move(compJson);
            }
        }

        m_json["components"] = std::move(json);
    }
}

const Json& SceneSerializer::GetJson() const
{
    return m_json;
}

}   // namespace jam