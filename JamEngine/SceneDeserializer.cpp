#include "pch.h"

#include "SceneDeserializer.h"

#include "ComponentDB.h"
#include "Entity.h"
#include "JsonReader.h"
#include "Profile.h"
#include "Scene.h"
#include "SceneDB.h"
#include "UID.h"

#define JAM_HANDLE_SCENE_DESERIALIZER_ERROR(...)                     \
    GlobalBuffer::SetLastError(__VA_ARGS__);                         \
    if (m_bAllowPartialSucceeded)                                    \
    {                                                                \
        Log::Error(GlobalBuffer::GetLastError()); /* just logging */ \
        continue;                                                    \
    }                                                                \
    else                                                             \
        return false

namespace jam
{

SceneDeserializer::SceneDeserializer(
    const bool _bAllowPartialSucceeded)
    : m_bAllowPartialSucceeded(_bAllowPartialSucceeded)
{
}

bool SceneDeserializer::LoadJsonFile(const fs::path& _path)
{
    return MoveIfHasValue(jam::LoadJsonFile(_path), m_json);
}

bool SceneDeserializer::LoadBsonFile(const fs::path& _path)
{
    return MoveIfHasValue(jam::LoadBsonFile(_path), m_json);
}

void SceneDeserializer::LoadJson(const Json& _json)
{
    m_json = _json;
}

bool SceneDeserializer::Deserialize()
{
    JAM_ASSERT(!m_json.empty(), "No data to deserialize");
    JAM_FUNCTION_PROFILE();

    // create reader
    JsonReader reader { m_json };

    // create scene
    {
        JsonReader sceneReader = {};
        if (!MoveIfHasValue(reader.EnterField("sceneType"), sceneReader))
        {
            return false;
        }

        // read scene type
        std::string sceneType;
        if (!sceneReader.Read(sceneType))
        {
            return false;
        }

        // create scene from SceneDB
        const SceneDB& db = SceneDB::GetInstance();
        if (!db.IsReflected(sceneType))
        {
            GlobalBuffer::SetLastError("Scene type '{}' is not reflected in SceneDB", sceneType);
            return false;
        }

        m_pScene = db.Create(sceneType);
    }

    // create entities
    {
        // entity reader
        JsonReader entityReader = {};
        if (!MoveIfHasValue(reader.EnterField("entities"), entityReader))
        {
            return false;
        }

        // entity UIDs
        const Entity root    = m_pScene->GetRootEntity();
        const UID    rootUid = root.GetUID();
        for (Int32 i = 0; i < entityReader.GetSize(); ++i)
        {
            UID uid;
            if (!entityReader.ReadAt(i, uid))
            {
                JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Failed to read entity UID at index: {}", i);
            }

            if (uid != rootUid)   // 루트 엔티티는 이미 존재하므로 스킵
            {
                (void)m_pScene->CreateEntity(uid);
            }
        }
    }

    // components [2]
    {
        JsonReader cmpsReader = {};
        if (!MoveIfHasValue(reader.EnterField("components"), cmpsReader))
        {
            return false;
        }

        const ComponentDB& db = ComponentDB::GetInstance();

        // components
        for (Int32 i = 0; i < cmpsReader.GetSize(); ++i)
        {
            // entry
            JsonReader entryReader;
            if (!MoveIfHasValue(cmpsReader.EnterAt(i), entryReader))
            {
                JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Failed to enter component entry at index: {}", i);
            }

            // type name
            std::string typeName;
            if (!entryReader.ReadAt(0, typeName))
            {
                JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Failed to read component type name at entry index: {}", i);
            }

            // check reflected type
            const ComponentInfo& info = db.GetComponentInfo(typeName);
            if (!db.IsReflected(typeName) || !info.serializeFn)
            {
                JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Component type ID {} at entry index {} is not reflected in ComponentDB", typeName, i);
            }

            // values
            JsonReader valuesReader = {};
            if (!MoveIfHasValue(entryReader.EnterAt(1), valuesReader))
            {
                JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Failed to enter component values at entry index: {}", i);
            }

            for (Int32 j = 0; j < valuesReader.GetSize(); ++j)
            {
                // value
                JsonReader valueReader = {};
                if (!MoveIfHasValue(valuesReader.EnterAt(j), valueReader))
                {
                    JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Failed to enter component value at entry index: {}, value index: {}", i, j);
                }

                // entity UID
                UID uid;
                if (!valueReader.ReadAt(0, uid))
                {
                    JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Failed to read entity UID at entry index: {}, value index: {}", i, j);
                }

                // get entity
                Entity entity = m_pScene->FindEntity(uid);
                if (!entity)
                {
                    JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Entity with UID {} not found for component at entry index: {}, value index: {}", uid, i, j);
                }

                // create component if not exists
                if (!info.hasFn(entity))
                {
                    info.createFn(entity);
                }

                // deserialize component
                if (void* pCmpValue = info.getFn(entity))
                {

                    // component data reader
                    JsonReader dataReader = {};
                    if (!MoveIfHasValue(valueReader.EnterAt(1), dataReader))
                    {
                        JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Failed to enter component data at entry index: {}, value index: {}", i, j);
                    }

                    info.deserializeFn(pCmpValue, dataReader, m_pScene, entity);

                    // check success
                    if (!dataReader.IsSucceeded())
                    {
                        JAM_HANDLE_SCENE_DESERIALIZER_ERROR("Failed to deserialize component type ID {} for entity UID {} at entry index: {}, value index: {}", typeName, uid, i, j);
                    }
                }
            }
        }
    }

    // 치명적 실패시 early return 하기 때문에
    // 이 시점에서는 부분 성공 or 완전 성공 상태임
    if (m_bAllowPartialSucceeded)
    {
        return true;   // 부분 성공 허용 모드면 무조건 성공 반환
    }
    else
    {
        return reader.IsSucceeded();   // 모든 과정이 완전하게 성공했는지 여부 반환
    }
}

Ref<Scene> SceneDeserializer::GetScene() const
{
    JAM_ASSERT(m_pScene, "Scene has not been deserialized yet");
    return m_pScene;
}

}   // namespace jam