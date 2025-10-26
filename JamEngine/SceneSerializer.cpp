#include "pch.h"

#include "SceneSerializer.h"

#include "ComponentDB.h"
#include "FileUtilities.h"
#include "Profile.h"
#include "Scene.h"
#include "SceneDB.h"

namespace jam
{

eErrorCode SceneSerializer::SaveToFile(const fs::path& _filePath) const
{
    JAM_ASSERT(!_filePath.empty(), "File path is not set");
    JAM_ASSERT(!m_json.empty(), "No data to export");
    return WriteJsonToFile(_filePath, m_json, 0);
}

eErrorCode SceneSerializer::LoadFromFile(const fs::path& _filePath)
{
    JAM_ASSERT(!_filePath.empty(), "File path is empty");
    auto [ec, json] = ReadJsonFromFile(_filePath);
    if (IsOk(ec))
        m_json = std::move(json);
    return ec;
}

void SceneSerializer::Serialize(const Scene* _pScene)
{
    JAM_ASSERT(_pScene, "Scene is null");
    JAM_PROFILE_SCOPE_MS("SceneSerializer::Serialize");

    m_json.clear();

    // 씬 정보 직렬화
    m_json["sceneName"] = _pScene->GetName();

    const HashMap<UID, Entity>& entityMap = _pScene->GetEntities();   // 전체 엔티티 맵
    HashMap<UInt32, UID>        entityUIDMap;                         // 엔티티 타입 해시 -> UID 맵 (직렬화용)
    entityUIDMap.reserve(entityMap.size());

    // 전체 엔티티 맵을 순회하면서
    // entity -> UID 맵 생성
    // 동시에 엔티티 리스트 직렬화
    std::vector<UID> entities;
    entities.reserve(entityMap.size());
    for (auto [uid, entity]: entityMap)
    {
        entities.push_back(uid);

        entt::entity handle                     = entity.GetHandle();
        entityUIDMap[entt::to_integral(handle)] = uid;
    }
    m_json["entities"] = std::move(entities);

    // 레지스트리 전체를 순회하며 컴포넌트 직렬화
    {
        Json json;   // 컴포넌트 데이터

        const entt::registry& pReg = _pScene->GetRegistry();
        for (auto [typeID, set]: pReg.storage())
        {
            if (!ComponentDB::IsSerializable(typeID))
                continue;

            Json compJson;   // 컴포넌트 데이터
            for (entt::entity handle: set)
            {
                Int32  id     = entt::to_integral(handle);
                UID    uid    = entityUIDMap.at(id);
                Entity entity = entityMap.at(uid);

                const void* pComponent = set.value(handle);
                Json        serialized = ComponentDB::Serialize(typeID, pComponent, _pScene, entity);
                if (serialized.is_null() || serialized.empty())
                    compJson[std::to_string(uid)] = nullptr;   // 빈 컴포넌트 처리
                else
                    compJson[std::to_string(uid)] = std::move(serialized);   // [컴포넌트 TYPE_ID][UID] = 컴포넌트 데이터
            }
            json[std::to_string(typeID)] = std::move(compJson);
        }
        m_json["components"] = std::move(json);
    }
}

Result<std::unique_ptr<Scene>> SceneSerializer::Deserialize()
{
    JAM_ASSERT(!m_json.empty(), "No data to deserialize");
    JAM_PROFILE_SCOPE_MS("SceneSerializer::Deserialize");

    if (!m_json.contains("sceneName") || !m_json["sceneName"].is_string())
    {
        SetErrorMessage("Scene JSON does not contain valid 'sceneName'");
        return eErrorCode::FileParseFailed;
    }

    if (!m_json.contains("sceneBehavior") || !m_json["sceneBehavior"].is_string())
    {
        SetErrorMessage("Scene JSON does not contain valid 'sceneBehavior'");
        return eErrorCode::FileParseFailed;
    }

    // 씬 생성
    std::string            sceneName = m_json["sceneName"].get<std::string>();
    std::unique_ptr<Scene> pScene    = SceneDB::Create(sceneName);

    // 엔티티 역직렬화
    if (!m_json.contains("entities") || !m_json["entities"].is_array())
    {
        SetErrorMessage("Scene JSON does not contain valid 'entities' array");
        return eErrorCode::FileParseFailed;
    }

    const Json& entitiesJson = m_json["entities"];
    for (const Json& uidJson: entitiesJson)
    {
        if (!uidJson.is_number_unsigned())
        {
            SetErrorMessage("Invalid UID in 'entities' array");
            return eErrorCode::FileParseFailed;
        }

        UID    entityUID = uidJson.get<UID>();
        Entity e         = pScene->CreateEntity(entityUID);
        JAM_UNUSED(e);
    }

    if (!m_json.contains("components") || !m_json["components"].is_object())
    {
        SetErrorMessage("Scene JSON does not contain valid 'components' object");
        return eErrorCode::FileParseFailed;
    }

    const Json& componentsJson = m_json["components"];
    for (auto& [typeIDStr, compJson]: componentsJson.items())
    {
        if (!compJson.is_object())
        {
            SetErrorMessage("Invalid component data for type ID: {}", typeIDStr);
            return eErrorCode::FileParseFailed;
        }

        auto [ec, typeID] = StringToInt<UInt32>(typeIDStr);
        if (IsError(ec))
            return ec;

        JAM_ASSERT(ComponentDB::IsSerializable(typeID), "Component type ID: {} is not serializable", typeID);
        for (auto& [uidStr, compData]: compJson.items())
        {
            auto [ec, uid] = StringToInt<UID>(uidStr);
            if (IsError(ec))
                return ec;

            UID    entityUID = uid;
            Entity entity    = pScene->FindEntity(entityUID);
            if (!entity.IsValid())
            {
                SetErrorMessage("Entity with UID: {} does not exist for component type ID: {}", entityUID, typeIDStr);
                return eErrorCode::FileParseFailed;
            }

            // 컴포넌트 생성 및 역직렬화
            if (!ComponentDB::Has(typeID, entity))
                ComponentDB::Create(typeID, entity);   // 컴포넌트 생성

            void* pComponent = ComponentDB::Get(typeID, entity);
            if (pComponent)   // 컴포넌트가 빈 구조체인 경우 nullptr일 수 있음
            {
                eErrorCode err = ComponentDB::Deserialize(typeID, pComponent, compData, pScene.get(), entity);
                if (IsError(err))
                {
                    ComponentDB::Remove(typeID, entity);   // 역직렬화 실패 시 컴포넌트 제거
                    return err;
                }
            }
        }
    }

    return eErrorCode::Ok;
}

}   // namespace jam