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
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Mat4, _11, _12, _13, _14, _21, _22, _23, _24, _31, _32, _33, _34, _41, _42, _43, _44);
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
    json["mtx"]    = local.GetMatrix();
    json["parent"] = t.GetParent().GetUID();
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
    Mat4 mat = _json["mtx"].get<Mat4>();
    transform.SetLocalMatrix(mat);

    // 부모 설정
    UID    parentUID = _json["parent"].get<UID>();
    Entity parent    = _pScene->FindEntity(parentUID);
    if (!parent.IsValid())
    {
        SetErrorMessage("TransformComponent deserialization failed: 'parent' entity with UID {} does not exist.", parentUID);
        return eErrorCode::FileParseFailed;
    }

    t.SetParent(parent, true);
    return eErrorCode::Ok;
}

Json CameraComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    Json json;
    json["projection"] = camera.GetProjection();
    json["nearZ"]      = camera.GetNearZ();
    json["farZ"]       = camera.GetFarZ();
    json["fovYRad"]    = camera.GetFovYRad();
    json["width"]      = camera.GetWidth();
    json["height"]     = camera.GetHeight();
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

    eProjection projection = static_cast<eProjection>(_json["projection"].get<Int32>());
    if (!IsValidEnum(projection))
        projection = eProjection::Perspective;

    camera.SetProjection(projection);
    camera.SetNearZ(_json["nearZ"].get<float>());
    camera.SetFarZ(_json["farZ"].get<float>());
    camera.SetFovYRad(_json["fovYRad"].get<float>());
    camera.SetWidth(_json["width"].get<float>());
    camera.SetHeight(_json["height"].get<float>());
    bMain = _json["bMain"].get<bool>();
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
