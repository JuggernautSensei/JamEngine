#include "pch.h"

#include "SceneDeserializer.h"

#include "ComponentDB.h"
#include "Entity.h"
#include "Profile.h"
#include "Scene.h"
#include "SceneDB.h"
#include "UID.h"

namespace jam
{

bool SceneDeserializer::LoadFile(const fs::path& _path)
{
    JAM_ASSERT(!_path.empty(), "file path is empty");
    if (std::optional<Json> opt = LoadBsonFile(_path))
    {
        m_json = std::move(*opt);
        return true;
    }
    else
    {
        return false;
    }
}

void SceneDeserializer::LoadJson(const Json& _json)
{
    m_json = _json;
}

bool SceneDeserializer::Deserialize()
{
    JAM_ASSERT(!m_json.empty(), "No data to deserialize");
    JAM_FUNCTION_PROFILE();

    // create scene
    {
        std::string sceneTypeName;
        if (!TryParseJsonField(m_json, "sceneType", sceneTypeName))
        {
            return false;   // core error
        }

        SceneDB& sceneDb = SceneDB::GetInstance();
        if (!sceneDb.IsReflected(sceneTypeName))
        {
            return false;   // core error
        }
        m_pScene = sceneDb.Create(sceneTypeName);
    }

    // entitis
    {
        Vector<UID> uids;
        if (!TryParseJsonField(m_json, "uids", uids))
        {
            return false;   // core error
        }

        const Entity root    = m_pScene->GetRootEntity();
        const UID    rootUid = root.GetUID();
        for (const UID uid: uids)
        {
            if (uid != rootUid)   // root is already created at scene constructor
            {
                (void)m_pScene->CreateEntity(uid);
            }
        }
    }

    // components
    {
        ComponentDB&        cmpDb = ComponentDB::GetInstance();
        std::optional<Json> opt   = FindJsonField(m_json, "cmps");
        if (!opt)
        {
            return false;   // core error
        }

        Json& json = *opt;
        for (const auto& [typeName, cmpJson]: json.items())
        {
            const std::optional<UInt32> optTypeId = TryParseStringToInt<UInt32>(typeName);
            if (!optTypeId)
            {
                return false;   // core error
            }

            const UInt32 typeId = *optTypeId;
            if (!cmpDb.IsReflected(typeId) || !cmpDb.IsSerializable(typeId))
            {
                return false;   // core error
            }

            for (const auto& [strUid, cmpValueJson]: cmpJson.items())
            {
                const std::optional<UInt32> optUid = TryParseStringToInt<UInt32>(strUid);
                if (!optUid)
                {
                    return false;   // core error
                }

                const UID    uid    = *optUid;
                const Entity entity = m_pScene->FindEntity(uid);
                if (!entity)
                {
                    ErrorBuffer::Set("Entity with UID: {} not found for component type ID: {}", uid, typeName);
                    return false;
                }

                // create component if not exists
                if (!cmpDb.Has(typeId, entity))
                {
                    cmpDb.Create(typeId, entity);
                }

                // deserialize
                if (void* pCmpValue = cmpDb.Get(typeId, entity))
                {
                    cmpDb.Deserialize(typeId, pCmpValue, cmpValueJson, m_pScene, entity);
                }
            }
        }
    }

    return true;
}

Ref<Scene> SceneDeserializer::GetScene() const
{
    JAM_ASSERT(m_pScene, "Scene has not been deserialized yet");
    return m_pScene;
}

}   // namespace jam