#include "pch.h"
//
#include "SceneSerializer.h"
//
#include "ComponentMetaManager.h"
#include "Config.h"
#include "Entity.h"
#include "JsonUtilities.h"
#include "ModelAsset.h"
#include "Scene.h"
#include "StringUtilities.h"
#include "TextureAsset.h"

namespace
{
using namespace jam;
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

    auto [json, bResult] = LoadJsonFromFile(_filePath);
    if (bResult)
    {
        m_json = std::move(json);
        return true;
    }
    JAM_ERROR("Failed to load scene from file: {}", _filePath.string());
    return false;
}

bool SceneSerializer::SaveFromFile(const fs::path& _filePath) const
{
    if (!IsValidSceneExtension(_filePath))
    {
        JAM_ERROR("Invalid scene file extension: {}. Expected: {}", ConvertToString(_filePath.extension().native()), ConvertToString(k_jamSceneExtensionW));
        return false;
    }

    if (!SaveJsonToFile(m_json, _filePath))
    {
        JAM_ERROR("Failed to save scene to file: {}", _filePath.string());
        return false;
    }
    return true;
}

void SceneSerializer::Serialize(Scene* _pScene)
{
    JAM_ASSERT(_pScene, "SceneSerializer::Serialize() - Scene pointer is null");

    // 이전 JSON 초기화
    Clear();

    // 에셋 직렬화
    {
        Json assetJson = SerializeAssetManager(_pScene->GetAssetManagerRef());
        if (IsValidJson(assetJson))
        {
            m_json["assets"] = std::move(assetJson);
        }
    }

    // 엔티티 직렬화
    {
        Json entityJson = SerializeEntity(_pScene);
        if (IsValidJson(entityJson))
        {
            m_json["entities"] = std::move(entityJson);
        }
    }

    // 컴포넌트 직렬화
    {
        Json componentJson = SerializeComponent(_pScene);
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

    _pScene->Clear();

    // 에셋 역직렬화
    if (m_json.contains("assets"))
    {
        const Json& assetJson = m_json["assets"];
        DeserializeAssetManager(assetJson, &_pScene->GetAssetManagerRef());
    }

    // 엔티티 역직렬화
    if (m_json.contains("entities"))
    {
        const Json& entityJson = m_json["entities"];
        DeserializeEntity(entityJson, _pScene);
    }

    // 컴포넌트 역직렬화
    if (m_json.contains("components"))
    {
        const Json& componentJson = m_json["components"];
        DeserializeComponent(componentJson, _pScene);
    }

    // 유저 데이터 역직렬화
    if (m_json.contains("userdata"))
    {
        const Json& userJson = m_json["userdata"];
        _pScene->OnDeserialize(userJson);
    }
}

void SceneSerializer::Clear()
{
    m_json.clear();
}

Json SceneSerializer::SerializeAssetManager(const AssetManager& _assetMgr) const
{
    Json outputJson;
    for (eAssetType type: EnumRange<eAssetType>())
    {
        Json        json;
        const auto& container = _assetMgr.GetContainer(type);
        for (const fs::path& key: container | std::views::keys)
        {
            json.push_back(key);
        }

        if (IsValidJson(json))
        {
            std::string_view typeName = EnumToString(type);
            outputJson[typeName]      = std::move(json);
        }
    }
    return outputJson;
}

Json SceneSerializer::SerializeEntity(Scene* _pScene) const
{
    Json outputJson;
    auto view = _pScene->CreateView<entt::entity>();
    for (entt::entity handle: view)
    {
        outputJson.push_back(entt::to_integral(handle));   // 엔티티 ID 직렬화
    }
    return outputJson;
}

Json SceneSerializer::SerializeComponent(Scene* _scene) const
{
    Json                  outputJson;
    const entt::registry& registry  = _scene->GetRegistry();
    const auto&           container = ComponentMetaManager::GetMetaContainer();   // 성능 향상을 위해 직접 컨테이너 접근

    // {[component name] : {[entity id] : serialized component}}
    for (auto&& [componentID, sparseSet]: registry.storage())
    {
        std::string_view     componentName = ComponentMetaManager::GetComponentNameByHash(componentID);
        const ComponentMeta& meta          = container.at(componentName);

        Json json;
        for (entt::entity handle: sparseSet)
        {
            if (meta.serializeComponentCallback)   // 직렬화 가능?
            {
                Entity entity          = _scene->GetEntity(handle);
                void*  pComponentValue = meta.getComponentCallback(entity);
                if (pComponentValue)   // 빈 struct 보호
                {
                    Json serializeJson = meta.serializeComponentCallback(pComponentValue);
                    if (IsValidJson(serializeJson))
                    {
                        json[std::to_string(entt::to_integral(handle))] = std::move(serializeJson);
                    }
                }
            }
        }

        if (IsValidJson(json))   // 비어있지 않으면 추가
        {
            outputJson[componentName] = std::move(json);
        }
    }

    return outputJson;
}

void SceneSerializer::DeserializeAssetManager(const Json& _json, AssetManager* _pAssetMgr) const
{
    JAM_ASSERT(_pAssetMgr, "DeserializeAssetManager() - AssetManager pointer is null");

    // JSON 형식 검사
    if (!_json.is_object())
    {
        JAM_ERROR("DeserializeAssetManager() - 'assets' JSON is not an object");
        return;
    }

    for (const eAssetType& type: EnumRange<eAssetType>())
    {
        std::string_view typeName = EnumToString(type);
        if (!_json.contains(typeName))
        {
            continue;
        }

        const Json& json = _json[typeName];
        if (!json.is_array())
        {
            JAM_ERROR("DeserializeAssetManager() - Asset JSON is not an array for type: {}", typeName);
            continue;
        }

        for (const Json& item: json)
        {
            std::filesystem::path path = item.get<std::filesystem::path>();

            switch (type)
            {
                static_assert(EnumCount<eAssetType>() == 2, "Add new asset type to SceneSerializer::DeserializeAssetManager()");

                case eAssetType::Model: _pAssetMgr->Load<ModelAsset>(path); break;
                case eAssetType::Texture: _pAssetMgr->Load<TextureAsset>(path); break;
                default:
                    JAM_ERROR("DeserializeAssetManager() - Unsupported asset type: {}", typeName);
                    break;
            }
        }
    }
}

void SceneSerializer::DeserializeEntity(const Json& _json, Scene* _pScene) const
{
    JAM_ASSERT(_pScene, "DeserializeEntity() - Scene pointer is null");

    // JSON 형식 검사
    if (!_json.is_array())
    {
        JAM_ERROR("DeserializeEntity() - 'entities' JSON is not an array");
        return;
    }

    // 엔티티 생성
    for (const Json& item: _json)
    {
        UInt32 id = item.get<UInt32>();
        Entity e  = _pScene->CreateEntity(id);
        UNUSED(e);
    }
}

void SceneSerializer::DeserializeComponent(const Json& _json, Scene* _pScene) const
{
    JAM_ASSERT(_pScene, "DeserializeComponent() - Scene pointer is null");

    // JSON 형식 검사
    if (!_json.is_object())
    {
        JAM_ERROR("DeserializeComponent() - 'components' JSON is not an object");
        return;
    }

    // 메타 컨테이너 접근
    const auto& metaContainer = ComponentMetaManager::GetMetaContainer();
    for (auto&& [componentKey, componentValuesJson]: _json.items())
    {
        // 메타 존재 확인
        auto metaIt = metaContainer.find(componentKey);
        if (metaIt == metaContainer.end())
        {
            JAM_ERROR("DeserializeComponent() - Component meta not found: {}", componentKey);
            continue;
        }
        const ComponentMeta& meta = metaIt->second;

        // 역직렬화 가능 여부 확인
        if (!meta.deserializeComponentCallback || !meta.createComponentCallback || !meta.getComponentCallback)
        {
            JAM_ERROR("DeserializeComponent() - Missing callbacks for component '{}'", componentKey);
            continue;
        }

        // 컴포넌트 값 반복
        if (!componentValuesJson.is_object())
        {
            JAM_ERROR("DeserializeComponent() - Component '{}' JSON is not an object", componentKey);
            continue;
        }

        for (auto&& [entityIdStr, componentValueJson]: componentValuesJson.items())
        {
            // 엔티티 조회
            UInt32 entityID = static_cast<UInt32>(std::stoul(entityIdStr));
            Entity entity   = _pScene->GetEntity(entityID);
            if (!entity.IsValid())
            {
                JAM_ERROR("DeserializeComponent() - Entity '{}' is not valid for component '{}'", entityID, componentKey);
                continue;
            }

            // 컴포넌트 생성
            meta.createComponentCallback(entity);

            // 컴포넌트 데이터 주소
            void* pComponentValue = meta.getComponentCallback(entity);
            if (!pComponentValue)   // 빈 struct일 수 있음
            {
                continue;
            }

            // 역직렬화
            meta.deserializeComponentCallback(componentValueJson, _pScene, entity, pComponentValue);
        }
    }
}

bool SceneSerializer::IsValidSceneExtension(const fs::path& _filePath) const
{
    std::wstring ext = ToLower(_filePath.extension().native());
    return ext == k_jamSceneExtensionW;
}

bool SceneSerializer::IsValidJson(const Json& _json) const
{
    return !_json.is_null() && !_json.empty();
}

}   // namespace jam