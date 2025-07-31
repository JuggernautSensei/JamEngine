#include "pch.h"

#include "Components.h"

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

void TagComponent::Deserialize(const ComponentDeserializeData& _deserializeData)
{
    const Json& json = _deserializeData.json;
    name             = json.value("name", "unknown");
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

void TransformComponent::Deserialize(const ComponentDeserializeData& _deserializeData)
{
    const Json& json = _deserializeData.json;
    position         = json.value("position", Vec3::Zero);
    rotation         = json.value("rotation", Quat::Identity);
    scale            = json.value("scale", Vec3::One);
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

void CameraComponent::Deserialize(const ComponentDeserializeData& _deserializeData)
{
    const Json& json = _deserializeData.json;
    fovY             = json.value("fovY", 45.f);
    nearZ            = json.value("nearZ", 0.1f);
    farZ             = json.value("farZ", 1000.f);
    aspectRatio      = json.value("aspectRatio", 1.f);
    projection       = json.value("projection", eProjection::Perspective);
    projection       = GetValidEnum(projection, eProjection::Perspective);
}

ScriptComponent::ScriptComponent(std::unique_ptr<Script>&& _script)
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

void ScriptComponent::Deserialize(const ComponentDeserializeData& _deserializeData)
{
    const Json& json       = _deserializeData.json;
    std::string scriptName = json.value("scriptName", "");
    if (!ScriptMetaManager::IsRegistered(scriptName))
    {
        JAM_ERROR("Script '{}' is not registered, you need to register it with JAM_SCRIPT macro", scriptName);
        return;
    }
    script = ScriptMetaManager::CreateScript(scriptName, Entity::s_null);
    JAM_ASSERT(script, "Failed to create script '{}'", scriptName);
}

}   // namespace jam