#include "pch.h"

#include "SceneSerializer.h"

#include "ComponentDB.h"
#include "JsonWriter.h"
#include "Profile.h"
#include "Scene.h"

namespace jam
{

bool SceneSerializer::SaveJsonFile(
    const fs::path& _path) const
{
    JAM_ASSERT(!_path.empty(), "file path is empty");
    JAM_ASSERT(!m_json.empty(), "no data to serialize");
    return jam::SaveJsonFile(_path, m_json);
}

bool SceneSerializer::SaveBsonFile(const fs::path& _path) const
{
    JAM_ASSERT(!_path.empty(), "file path is empty");
    JAM_ASSERT(!m_json.empty(), "no data to serialize");
    return jam::SaveBsonFile(_path, m_json);
}

void SceneSerializer::Serialize(
    const Ref<Scene>& _pScene)
{
    JAM_ASSERT(_pScene, "scene is null");
    JAM_FUNCTION_MSG_PROFILE("scene name: {}", _pScene->GetTypeName());

    // scene data
    const HashMap<UID, Entity>& entities    = _pScene->GetEntities();
    const Int32&                numEntities = _pScene->GetNumEntities();

    // clean up
    m_json.clear();

    // start
    JsonWriter writer { m_json };

    // write scene type
    writer.WriteField("sceneType", _pScene->GetTypeName());

    // [handle, UID] map (entt::entity 와 uid 추적용)
    HashMap<UInt32, UID> handleUidMap;
    handleUidMap.reserve(numEntities);

    // gather entities + write all entities
    {
        // gather
        Vector<UID> uids;
        uids.reserve(numEntities);
        for (const auto& [uid, entity]: entities)
        {
            uids.emplace_back(uid);

            const UInt32 iHandle  = entt::to_integral(entity.GetHandle());
            handleUidMap[iHandle] = uid;
        }

        // write entities
        writer.WriteField("entities", std::move(uids));
    }

    // components
    // ㄴ array of component entries
    // component entry
    // ㄴ [ typeName, [ array of component values ] ]
    // component value
    // ㄴ [ uid, data ]
    {
        JsonWriter         cmpsWriter = writer.WriteField("components");   // components array
        const ComponentDB& db         = ComponentDB::GetInstance();

        // iterate all reflected component types
        const entt::registry& reg = _pScene->GetRegistry();
        for (const auto& [typeId, storage]: reg.storage())
        {
            // can serialize?
            // skip non-reflected or non-serializable or components that have no instances
            if (!db.IsReflected(typeId) || !db.IsSerializable(typeId) || storage.empty())
            {
                continue;
            }

            // entry
            JsonWriter entryWriter = cmpsWriter.PushBack();

            StringView typeName = db.GetComponentName(typeId);
            entryWriter.PushBack(typeName);   // type name
            entryWriter.PushBack();           // array of component values

            // fill values
            JsonWriter valuesWriter = entryWriter.EnterBack();
            for (const entt::entity handle: storage)
            {
                const UInt32 iHandle = entt::to_integral(handle);
                JAM_ASSERT(handleUidMap.contains(iHandle), "entity UID not found for handle: {}", iHandle);

                // find entity by handle
                const UID    uid       = handleUidMap.at(iHandle);
                const Entity entity    = entities.at(uid);
                const void*  pCmpValue = storage.value(handle);

                // value
                JsonWriter valueWriter = valuesWriter.PushBack();
                valueWriter.PushBack(uid);   // entity uid
                valueWriter.PushBack();      // component data

                // serialize component data
                JsonWriter dataWriter = valueWriter.EnterBack();
                db.Serialize(typeId, pCmpValue, dataWriter, _pScene, entity);
            }
        }
    }
}

const Json& SceneSerializer::GetJson() const
{
    return m_json;
}

}   // namespace jam