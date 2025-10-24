#include "pch.h"

#include "Components.h"

#include "AssetManager.h"
#include "Scene.h"
#include "Script.h"
#include "ScriptDB.h"

namespace nlohmann
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Vec3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Quat, x, y, z, w);
}   // namespace nlohmann

namespace jam
{

Json TagComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    Json json;
    json["name"] = name;
    return json;
}

eErrorCode TagComponent::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    // 유효성 체크
    if (!_json.contains("name"))
    {
        SetErrorMessage("TagComponent deserialization failed: 'name' field is missing.");
        return eErrorCode::FileParseFailed;
    }

    name = _json["name"].get<std::string>();
    return eErrorCode::Ok;
}

Json TransformComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    const TransformHierarchy& t = _entity.GetTransform();

    JAM_ASSERT(t.GetRoot().IsValid(), "root entity is invalid");
    JAM_ASSERT(t.GetParent().IsValid(), "parent entity is invalid");
    JAM_ASSERT(t.GetOwner().IsValid(), "owner entity is invalid");

    const Transform& local = transform.GetLocal();

    Json json;
    json["translate"] = local.translate;
    json["rotate"]    = local.rotate;
    json["scale"]     = local.scale;
    json["parent"]    = t.GetParent().GetUID();
    return json;
}

eErrorCode TransformComponent::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    TransformHierarchy& t = _entity.GetTransform();
    JAM_ASSERT(t.GetRoot().IsValid(), "root entity is invalid");
    JAM_ASSERT(t.GetOwner().IsValid(), "owner entity is invalid");

    // 유효성 체크
    const char* fieldNames[] = { "translate", "rotate", "scale", "parent" };
    for (const char* fieldName: fieldNames)
    {
        if (!_json.contains(fieldName))
        {
            SetErrorMessage("TransformComponent deserialization failed: {} field is missing.", fieldName);
            return eErrorCode::FileParseFailed;
        }
    }

    // 로컬
    Transform local;
    local.translate = _json.value<Vec3>("translate", Vec3::Zero);
    local.rotate    = _json.value<Quat>("rotate", Quat::Identity);
    local.scale     = _json.value<Vec3>("scale", Vec3::One);
    t.SetLocal(local);

    // 부모 설정
    UID    parentUID = _json["parent"].get<UID>();
    Entity parent    = _pScene->FindEntity(parentUID);
    if (!parent.IsValid())
    {
        SetErrorMessage("TransformComponent deserialization failed: 'parent' entity with UID {} does not exist.", parentUID);
        return eErrorCode::FileParseFailed;
    }

    t.SetParent(parent);
    return eErrorCode::Ok;
}

Json CameraComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    Json json;
    json["projection"] = camera.projection;
    json["nearZ"]      = camera.nearZ;
    json["farZ"]       = camera.farZ;
    json["fovYRad"]    = camera.fovYRad;
    json["width"]      = camera.width;
    json["height"]     = camera.height;
    json["bMain"]      = bMain;
    return json;
}

eErrorCode CameraComponent::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    constexpr const char* const fieldNames[] = { "projection", "nearZ", "farZ", "fovYRad", "width", "height", "bMain" };
    for (const char* fieldName: fieldNames)
    {
        if (!_json.contains(fieldName))
        {
            SetErrorMessage("CameraComponent deserialization failed: {} field is missing.", fieldName);
            return eErrorCode::FileParseFailed;
        }
    }

    camera.projection = _json["projectection"].get<eProjection>();
    camera.nearZ      = _json["nearZ"].get<float>();
    camera.farZ       = _json["farZ"].get<float>();
    camera.fovYRad    = _json["fovYRad"].get<float>();
    camera.width      = _json["width"].get<float>();
    camera.height     = _json["height"].get<float>();
    bMain             = _json["bMain"].get<bool>();

    // 사후검사
    if (!IsValidEnum(camera.projection))
    {
        SetErrorMessage("CameraComponent deserialization failed: 'projection' field is invalid.");
        return eErrorCode::FileParseFailed;
    }

    camera.nearZ = Clamp(camera.nearZ, Camera::k_nearestZ, Camera::k_farthestZ - Camera::k_nearestZ);
    camera.farZ  = Clamp(camera.farZ, camera.nearZ + Camera::k_nearestZ, Camera::k_farthestZ);
    return eErrorCode::Ok;
}

Json ScriptComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    if (pScript)
    {
        Json json;
        json["script"] = pScript->GetName();
        return json;
    }
    else
    {
        Json json;
        json["script"] = nullptr;
        return json;
    }
}

eErrorCode ScriptComponent::Deserialize(const Json& _json, const Scene* _pScene, const Entity _entity)
{
    if (!_json.contains("script"))
    {
        SetErrorMessage("ScriptComponent deserialization failed: 'script' field is missing.");
        return eErrorCode::FileParseFailed;
    }

    Json scriptField = _json["script"];
    if (scriptField.is_null())
    {
        return eErrorCode::Ok;   // 스크립트 없음
    }
    else if (scriptField.is_string())
    {
        std::string scriptName = scriptField.get<std::string>();
        pScript                = ScriptDB::Create(scriptName, _entity);
        return eErrorCode::Ok;
    }

    SetErrorMessage("ScriptComponent deserialization failed: 'script' field is not a string or null.");
    return eErrorCode::FileParseFailed;
}

Json MeshRenderer::Serialize(const Scene* _pScene, Entity _entity) const
{
    Json json;

    // mesh field
    if (pMesh)
    {
        json["mesh"] = pMesh->GetPath();
    }
    else
    {
        json["mesh"] = nullptr;
    }

    // material field
    if (pMaterial)
    {
        json["material"] = pMaterial->GetPath();
    }
    else
    {
        json["material"] = nullptr;
    }

    return json;
}
eErrorCode MeshRenderer::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    constexpr const char* const fieldNames[] = { "mesh", "material" };
    for (const char* fieldName: fieldNames)
    {
        if (!_json.contains(fieldName))
        {
            SetErrorMessage("MeshRenderer deserialization failed: {} field is missing.", fieldName);
            return eErrorCode::FileParseFailed;
        }
    }

    // mesh field
    Json meshField = _json["mesh"];
    if (meshField.is_null())
    {
        pMesh = nullptr;
    }
    else if (meshField.is_string())
    {
        std::string meshPath  = meshField.get<std::string>();
        auto [ec, meshHandle] = AssetManager::GetOrLoadMesh(meshPath);

        if (IsError(ec))
            return ec;

        pMesh = meshHandle;
    }
    else
    {
        SetErrorMessage("MeshRenderer deserialization failed: 'mesh' field is not a string or null.");
        return eErrorCode::FileParseFailed;
    }

    // material field
    Json materialField = _json["material"];
    if (materialField.is_null())
    {
        pMaterial = nullptr;
    }
    else if (materialField.is_string())
    {
        std::string materialPath  = materialField.get<std::string>();
        auto [ec, materialHandle] = AssetManager::GetOrLoadMaterial(materialPath);

        if (IsError(ec))
            return ec;

        pMaterial = materialHandle;
    }
    else
    {
        SetErrorMessage("MeshRenderer deserialization failed: 'material' field is not a string or null.");
        return eErrorCode::FileParseFailed;
    }
    return eErrorCode::Ok;
}

}   // namespace jam
