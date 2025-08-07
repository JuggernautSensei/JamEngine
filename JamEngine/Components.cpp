#include "pch.h"

#include "Components.h"

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

void TagComponent::Deserialize(const DeserializeParameter& _param)
{
    name = _param.pJson->value("name", "unknown");
}

Mat4 TransformComponent::CreateWorldMatrix() const
{
    return jam::CreateWorldMatrix(position, rotation, scale);
}

Json TransformComponent::Serialize() const
{
    Json json;
    json["position"] = { position.x, position.y, position.z };
    json["rotation"] = { rotation.x, rotation.y, rotation.z, rotation.w };
    json["scale"]    = { scale.x, scale.y, scale.z };
    return json;
}

void TransformComponent::Deserialize(const DeserializeParameter& _param)
{
    const Json* json = _param.pJson;
    position         = json->value("position", Vec3::Zero);
    rotation         = json->value("rotation", Quat::Identity);
    scale            = json->value("scale", Vec3::One);
}

Mat4 CameraComponent::CreateViewMatrix(const Vec3& _position, const Vec3& _forward) const
{
    return jam::CreateViewMatrix(_position, _forward);
}

Mat4 CameraComponent::CreateProjectionMatrix() const
{
    if (projection == eProjection::Perspective)
    {
        return CreatePerspectiveMatrix(fovY, aspectRatio, nearZ, farZ);
    }
    else
    {
        return CreateOrthographicMatrix(aspectRatio, nearZ, farZ);
    }
}

Json CameraComponent::Serialize() const
{
    Json json;
    json["fovY"]        = fovY;
    json["nearZ"]       = nearZ;
    json["farZ"]        = farZ;
    json["aspectRatio"] = aspectRatio;
    json["projection"]  = projection;
    return json;
}

void CameraComponent::Deserialize(const DeserializeParameter& _param)
{
    const Json* json = _param.pJson;
    fovY             = json->value("fovY", 45.f);
    nearZ            = json->value("nearZ", 0.1f);
    farZ             = json->value("farZ", 1000.f);
    aspectRatio      = json->value("aspectRatio", 1.f);
    projection       = json->value("projection", eProjection::Perspective);
    projection       = GetValidEnumOrDefault(projection, eProjection::Perspective);
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

void ScriptComponent::Deserialize(const DeserializeParameter& _param)
{
    const Json* json = _param.pJson;
    if (json->contains("scriptName"))
    {
        std::string scriptName = json->at("scriptName").get<std::string>();
        if (ScriptMetaManager::IsRegistered(scriptName))
        {
            script = ScriptMetaManager::CreateScript(scriptName, *_param.pOwner);
        }
        return;
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

void ModelComponent::Deserialize(const DeserializeParameter& _param)
{
    JAM_ASSERT(_param.pScene, "Scene must not be null when deserializing ModelComponent");

    fs::path                                   path  = _param.pJson->value("path", std::filesystem::path());
    AssetManager&                              mgr   = _param.pScene->GetAssetManager();
    std::optional<Ref<ModelAsset>> model = mgr.GetModel(path);

    if (!model)
    {
        model = mgr.LoadModel(path);
    }

    if (model)
    {
        modelAsset = *model;
    }
}

}   // namespace jam