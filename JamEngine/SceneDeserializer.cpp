#include "pch.h"

#include "SceneDeserializer.h"

#include "ComponentDB.h"
#include "Entity.h"
#include "FileUtilities.h"
#include "Profile.h"
#include "Scene.h"
#include "SceneDB.h"
#include "UID.h"

namespace jam
{

bool SceneDeserializer::LoadFromFile(const fs::path& _filePath)
{
    JAM_ASSERT(!_filePath.empty(), "File path is empty");
    std::optional<Json> json = LoadJsonFromFile(_filePath);
    if (json)
    {
        m_json = std::move(json.value());
        return true;
    }
    else
    {
        return false;
    }
}

void SceneDeserializer::LoadFromJson(const Json& _json)
{
    m_json = _json;
}

bool SceneDeserializer::Deserialize()
{
    JAM_ASSERT(!m_json.empty(), "No data to deserialize");
    JAM_FUNCTION_PROFILE();

    try
    {
        const std::string name = m_json.at("sceneType").get<std::string>();
        m_pScene               = SceneDB::GetInstance().Create(name);

        // entities
        const Json& uidsJson = m_json.at("uids");
        for (const Json& uidJson: uidsJson)
        {
            const UID uid = uidJson.get<UID>();
            if (uid != 0)
            {
                // uid 0 is reserved for root entity
                const Entity unused = m_pScene->CreateEntity(uid);
                (void)unused;
            }
        }

        // components
        ComponentDB& compDB         = ComponentDB::GetInstance();
        const Json&  componentsJson = m_json.at("components");
        for (const auto& [strTypeID, compJson]: componentsJson.items())
        {
            const UInt32 typeID = std::stoul(strTypeID);
            JAM_ASSERT(compDB.IsSerializable(typeID), "Component type ID: {} is not serializable", strTypeID);
            for (const auto& [strUID, compDataJson]: compJson.items())
            {
                const UID    uid    = std::stoul(strUID);
                const Entity entity = m_pScene->FindEntity(uid);
                if (!entity)
                {
                    SetErrorString("Entity with UID: {} not found for component type ID: {}", uid, strTypeID);
                    return false;
                }

                // create component if not exists
                if (!compDB.Has(typeID, entity))
                {
                    compDB.Create(typeID, entity);
                }

                // deserialize
                void* pComp = compDB.Get(typeID, entity);
                if (pComp)
                {
                    // if pComp is nullptr, it means the component type does not have data (e.g., empty component)
                    if (!compDB.Deserialize(typeID, pComp, compDataJson, m_pScene, entity))
                    {
                        compDB.Remove(typeID, entity);   // remove component if deserialization failed
                        return false;
                    }
                }
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        SetErrorString("Scene deserialization failed: {}", e.what());
        return false;
    }
    catch (...)
    {
        SetErrorString("Scene deserialization failed with unknown error");
        return false;
    }
}

Ref<Scene> SceneDeserializer::GetScene() const
{
    JAM_ASSERT(m_pScene, "Scene has not been deserialized yet");
    return m_pScene;
}

}   // namespace jam