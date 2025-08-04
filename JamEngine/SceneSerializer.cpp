#include "pch.h"
//
#include "SceneSerializer.h"
//
#include "ComponentMetaManager.h"
#include "Config.h"
#include "Entity.h"
#include "JsonUtilities.h"
#include "Scene.h"
#include "StringUtilities.h"

namespace
{
using namespace jam;

NODISCARD bool IsValidSceneExtension(const fs::path& _filePath)
{
    std::wstring ext = ToLower(_filePath.extension().native());
    return ext == k_jamSceneExtensionW;
}

NODISCARD bool IsValidJson(const Json& _json)
{
    return !_json.is_null() && !_json.empty();
}

}   // namespace

namespace jam
{

bool SceneSerializer::LoadFromFile(const fs::path& _filePath)
{
    if (!IsValidSceneExtension(_filePath))
    {
        JAM_ERROR("Invalid scene file extension: {}. Expected: {}", ConvertToString(_filePath.extension().native()), ConvertToString(k_jamSceneExtensionW));
        return false;
    }

    std::optional<Json> json = LoadJsonFromFile(_filePath);
    if (json)
    {
        m_json = std::move(*json);
        return true;
    }
    else
    {
        JAM_ERROR("Failed to load scene from file: {}", _filePath.string());
        return false;
    }
}

bool SceneSerializer::SaveFromFile(const fs::path& _filePath) const
{
    if (!IsValidSceneExtension(_filePath))
    {
        JAM_ERROR("Invalid scene file extension: {}. Expected: {}", ConvertToString(_filePath.extension().native()), ConvertToString(k_jamSceneExtensionW));
        return false;
    }

    if (m_json.empty())
    {
        JAM_ERROR("Scene JSON is empty. Cannot save to file: {}", _filePath.string());
        return false;
    }

    if (!SaveJsonToFile(m_json, _filePath))
    {
        JAM_ERROR("Failed to save scene to file: {}", _filePath.string());
        return false;
    }

    return true;
}

void SceneSerializer::Serialize(const Scene* _pScene)
{
    JAM_ASSERT(_pScene, "SceneSerializer::Serialize() - Scene pointer is null");

    // clear previous JSON
    Clear();

    // serialize asset
    {
        // array of asset paths grouped by type
        const AssetManager& assetMgr = _pScene->GetAssetManager();
        Json                assetJson;
        for (eAssetType type: EnumValues<eAssetType>())
        {
            Json json;
            for (const fs::path& key: std::views::keys(assetMgr.GetContainer(type)))
            {
                json.push_back(key);
            }
            assetJson[EnumToString(type)] = std::move(json);
        }

        if (IsValidJson(assetJson))
        {
            m_json["assets"] = std::move(assetJson);
        }
    }

    // serialize entity
    {
        const entt::registry& registry = _pScene->GetRegistry();

        // serialize entity
        // array of entity IDs
        Json entityJson;
        for (entt::entity id: registry.view<entt::entity>())
        {
            entityJson.push_back(std::to_string(entt::to_integral(id)));
        }

        if (IsValidJson(entityJson))
        {
            m_json["entities"] = std::move(entityJson);
        }
    }

    // serialize components
    {
        const entt::registry& registry = _pScene->GetRegistry();

        // 성능 향상을 위해 직접 컨테이너에 접근
        const auto& metaContainer = ComponentMetaManager::GetMetaContainer();

        // serialize components
        // object of {[component name] : {[entity id] : [component data]}}
        Json componentJson;
        for (const auto& [componentID, enttContainer]: registry.storage())
        {
            std::string_view     componentName = ComponentMetaManager::GetComponentNameByHash(componentID);
            const ComponentMeta& meta          = metaContainer.at(componentName);

            Json json;
            for (entt::entity id: enttContainer)
            {
                if (meta.serializeComponentCallback)   // 직렬화가 가능한가?
                {
                    // 만약 직렬화가 가능하다면 데이터를 직렬화
                    Entity      entity = _pScene->GetEntity(id);
                    const void* value  = meta.getComponentOrNullCallback(entity);   // 컴포넌트의 값

                    // value 가 nullptr인 경우는 해당 컴포넌트가 빈 struct 이라서 직렬화가 불가능한 경우. 혹은 컴포넌트를 엔티티가 가지고 있지 않은 경우
                    JAM_ASSERT(value, "SceneSerializer::Serialize() - Component '{}' is not attached to entity with ID {}", componentName, entt::to_integral(id));
                    if (value)
                    {
                        Json serializeJson = meta.serializeComponentCallback(value);
                        if (IsValidJson(serializeJson))
                        {
                            json[std::to_string(entt::to_integral(id))] = std::move(serializeJson);
                        }
                    }
                }
                // 만약 직렬화가 불가능하다면 해당 컴포넌트는 무시
            }

            // 직렬화 된 경우에만 json에 추가
            // 만약 아무 데이터도 직렬화 되지 않았다면 해당 컴포넌트는 무시
            if (IsValidJson(json))
            {
                componentJson[componentName] = std::move(json);
            }
        }

        if (IsValidJson(componentJson))
        {
            m_json["components"] = std::move(componentJson);
        }
    }

    // 유저 데이터 직렬화
    {
        Json userJson = _pScene->OnSerialize();
        if (IsValidJson(userJson))
        {
            m_json["userdata"] = std::move(userJson);
        }
    }
}

void SceneSerializer::Deserialize(Scene* _pScene)
{
    JAM_ASSERT(_pScene, "SceneSerializer::Deserialize() - Scene pointer is null");
    JAM_ASSERT(m_json.empty() == false || m_json.is_null() == false, "SceneSerializer::Deserialize() - JSON is empty or null");

    _pScene->Clear();

    // 에셋 역직렬화
    {
        if (m_json.contains("assets"))
        {
            const Json&   assetJson = m_json["assets"];
            AssetManager& assetMgr  = _pScene->GetAssetManager();

            for (const eAssetType& type: EnumValues<eAssetType>())
            {
                if (assetJson.contains(EnumToString(type)))
                {
                    // pJson is array of asset paths
                    const Json& json = assetJson[EnumToString(type)];
                    JAM_ASSERT(json.is_array(), "SceneSerializer::Deserialize() - Asset JSON is not an array for type: {}", EnumToString(type));

                    for (const Json& item: json)
                    {
                        std::filesystem::path path = item.get<std::filesystem::path>();

                        switch (type)
                        {
                            case eAssetType::Model: assetMgr.LoadModel(path); break;
                            case eAssetType::Texture: assetMgr.LoadTexture(path); break;
                            default:
                                JAM_ERROR("SceneSerializer::Deserialize() - Unsupported asset type: {}", EnumToString(type));
                                continue;
                        }
                    }
                }
            }
        }
    }

    // entity deserialization
    {
        // create entities
        if (m_json.contains("entities"))
        {
            const Json& entityJson = m_json["entities"];
            JAM_ASSERT(entityJson.is_array(), "SceneSerializer::Deserialize() - Entity JSON is not an array");
            for (const Json& item: entityJson)
            {
                MAYBE_UNUSED Entity e = _pScene->CreateEntity(item.get<UInt32>());
            }
        }

        // component deserialization
        if (m_json.contains("components"))
        {
            // 성능 향상을 위해 직접 컨테이너에 접근
            const auto& metaContainer  = ComponentMetaManager::GetMetaContainer();
            const Json& componentsJson = m_json["components"];
            JAM_ASSERT(componentsJson.is_object(), "SceneSerializer::Deserialize() - Components JSON is not an object");
            for (const auto& [componentName, componentData]: componentsJson.items())
            {
                const ComponentMeta& meta = metaContainer.at(componentName);

                for (const auto& [entityId, componentValue]: componentData.items())
                {
                    if (meta.deserializeComponentCallback)   // 역직렬화가 가능한가?
                    {
                        // get entity by ID
                        Entity entity = _pScene->GetEntity(std::stoul(entityId));
                        if (entity.IsValid())
                        {
                            // 컴포넌트 생성
                            meta.createComponentCallback(entity);

                            // 컴포넌트 데이터
                            void* value = meta.getComponentOrNullCallback(entity);

                            // 역직렬화
                            DeserializeParameter param = {
                                .pJson        = &componentValue,
                                .pScene       = _pScene,
                                .pOnwerEntity = &entity
                            };
                            meta.deserializeComponentCallback(param, value);
                        }
                    }
                }
            }
        }
    }

    // 유저 데이터 역직렬화
    {
        if (m_json.contains("userdata"))
        {
            const Json& userJson = m_json["userdata"];
            _pScene->OnDeserialize(userJson);
        }
    }
}

void SceneSerializer::Clear()
{
    m_json.clear();
}

}   // namespace jam