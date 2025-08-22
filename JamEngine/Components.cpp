#include "pch.h"

#include "Components.h"

#include "JsonUtilities.h"
#include "ModelAsset.h"
#include "Script.h"
#include "ScriptMetaManager.h"

namespace nlohmann
{

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Vec3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Quat, x, y, z, w);

}   // namespace nlohmann

namespace jam
{

TagComponent::TagComponent(const std::string_view _name)
    : name(_name)
{
}

Json TagComponent::Serialize() const
{
    Json json;
    json["name"] = name;
    return json;
}

void TagComponent::Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity)
{
    name = GetJsonValueOrDefault(_json, "name", std::string("unknown"));
}

Mat4 TransformComponent::CreateWorldMatrix() const
{
    return jam::CreateWorldMatrix(position, rotation, scale);
}

Json TransformComponent::Serialize() const
{
    Json json;
    json["position"] = position;
    json["rotation"] = rotation;
    json["scale"]    = scale;
    return json;
}

void TransformComponent::Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity)
{
    position = GetJsonValueOrDefault(_json, "position", Vec3::Zero);
    rotation = GetJsonValueOrDefault(_json, "rotation", Quat::Identity);
    scale    = GetJsonValueOrDefault(_json, "scale", Vec3::One);
}

Mat4 CameraComponent::CreateViewMatrix(const Vec3& _position, const Vec3& _forward) const
{
    return jam::CreateViewMatrix(_position, _forward);
}

Mat4 CameraComponent::CreateProjectionMatrix() const
{
    if (projection == eProjection::Perspective)
    {
        return CreatePerspectiveMatrix(fovYRad, aspectRatio, nearZ, farZ);
    }
    else
    {
        return CreateOrthographicMatrix(aspectRatio, nearZ, farZ);
    }
}

Json CameraComponent::Serialize() const
{
    Json json;
    json["fovYRad"]     = fovYRad;
    json["nearZ"]       = nearZ;
    json["farZ"]        = farZ;
    json["aspectRatio"] = aspectRatio;
    json["projection"]  = projection;
    json["primary"]     = bPrimary;   // 추가: 기본 카메라 여부
    return json;
}

void CameraComponent::Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity)
{
    fovYRad     = GetJsonValueOrDefault(_json, "fovYRad", 45.f);
    nearZ       = GetJsonValueOrDefault(_json, "nearZ", 0.1f);
    farZ        = GetJsonValueOrDefault(_json, "farZ", 1000.f);
    aspectRatio = GetJsonValueOrDefault(_json, "aspectRatio", 1.f);
    projection  = GetJsonValueOrDefault(_json, "projection", eProjection::Perspective);
    bPrimary    = GetJsonValueOrDefault(_json, "primary", false);
}

ScriptComponent::ScriptComponent(Scope<Script>&& _script)
    : script(std::move(_script))
{
}

Json ScriptComponent::Serialize() const
{
    Json json;
    if (script)
    {
        json["scriptName"] = script->GetName();
    }
    return json;
}

void ScriptComponent::Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity)
{
    if (_json.contains("scriptName"))
    {
        std::string scriptName = _json["scriptName"];
        if (ScriptMetaManager::IsRegistered(scriptName))
        {
            script = ScriptMetaManager::CreateScript(scriptName, _onwerEntity);
        }
        else
        {
            JAM_ERROR("Script '{}' is not registered in ScriptMetaManager", scriptName);
        }
    }
}

Json ModelComponent::Serialize() const
{
    Json json;
    if (modelAsset)
    {
        json["path"] = modelAsset->GetPath();
    }
    return json;
}

void ModelComponent::Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity)
{
    if (_json.contains("path"))
    {
        fs::path path = _json["path"].get<fs::path>();

        AssetManager& mgrRef  = _pScene->GetAssetManagerRef();
        auto [model, bResult] = mgrRef.GetOrLoad<ModelAsset>(path);
        if (bResult)
        {
            modelAsset = model;
        }
    }
}

}   // namespace jam