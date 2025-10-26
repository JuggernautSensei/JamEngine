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
    json["projection"] = projection;
    json["nearZ"]      = nearZ;
    json["farZ"]       = farZ;
    json["fovYRad"]    = fovYRad;
    json["width"]      = width;
    json["height"]     = height;
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

    projection = _json["projectection"].get<eProjection>();
    nearZ      = _json["nearZ"].get<float>();
    farZ       = _json["farZ"].get<float>();
    fovYRad    = _json["fovYRad"].get<float>();
    width      = _json["width"].get<float>();
    height     = _json["height"].get<float>();
    bMain             = _json["bMain"].get<bool>();

    // 사후검사
    if (!IsValidEnum(projection))
    {
        SetErrorMessage("CameraComponent deserialization failed: 'projection' field is invalid.");
        return eErrorCode::FileParseFailed;
    }

    nearZ = Clamp(nearZ, k_nearestZ, k_farthestZ - k_epsilon);
    farZ  = Clamp(farZ, nearZ + k_epsilon, k_farthestZ);
    return eErrorCode::Ok;
}

Mat4 CameraComponent::CreateProj() const
{
    if (projection == eProjection::Perspective)
    {
        const float aspectRatio = width / height;
        return CreatePerspectiveFov(fovYRad, aspectRatio, nearZ, farZ);
    }
    else
    {
        return CreateOrthographic(width, height, nearZ, farZ);
    }
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

Json PointLightComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    Json json;
    json["radiance"]     = radiance;
    json["intensity"]    = intensity;
    json["fallOffStart"] = fallOffStart;
    json["fallOffEnd"]   = fallOffEnd;
    return json;
}

eErrorCode PointLightComponent::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    constexpr const char* const fieldNames[] = { "radiance", "intensity", "fallOffStart", "fallOffEnd" };
    for (const char* fieldName: fieldNames)
    {
        if (!_json.contains(fieldName))
        {
            SetErrorMessage("PointLightComponent deserialization failed: {} field is missing.", fieldName);
            return eErrorCode::FileParseFailed;
        }
    }

    radiance     = _json["radiance"].get<Vec3>();
    intensity    = _json["intensity"].get<float>();
    fallOffStart = _json["fallOffStart"].get<float>();
    fallOffEnd   = _json["fallOffEnd"].get<float>();
    return eErrorCode::Ok;
}

Json SpotLightComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    Json json;
    json["radiance"]         = radiance;
    json["intensity"]        = intensity;
    json["fallOffStart"]     = fallOffStart;
    json["fallOffEnd"]       = fallOffEnd;
    json["innerConeHalfRad"] = innerConeHalfRad;
    json["outerConeHalfRad"] = outerConeHalfRad;
    return json;
}

eErrorCode SpotLightComponent::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    constexpr const char* const fieldNames[] = { "radiance", "intensity", "fallOffStart", "fallOffEnd", "innerConeHalfRad", "outerConeHalfRad" };
    for (const char* fieldName: fieldNames)
    {
        if (!_json.contains(fieldName))
        {
            SetErrorMessage("SpotLightComponent deserialization failed: {} field is missing.", fieldName);
            return eErrorCode::FileParseFailed;
        }
    }

    radiance         = _json["radiance"].get<Vec3>();
    intensity        = _json["intensity"].get<float>();
    fallOffStart     = _json["fallOffStart"].get<float>();
    fallOffEnd       = _json["fallOffEnd"].get<float>();
    innerConeHalfRad = _json["innerConeHalfRad"].get<float>();
    outerConeHalfRad = _json["outerConeHalfRad"].get<float>();
    return eErrorCode::Ok;
}

Json ShadowCasterComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    return nullptr;
}

eErrorCode ShadowCasterComponent::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity) const
{
    return eErrorCode::Ok;
}

}   // namespace jam
