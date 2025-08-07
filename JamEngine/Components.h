#pragma once
#include "ComponentMetaManager.h"
#include "Script.h"

namespace jam
{

struct TagComponent : ISerializableComponent<TagComponent>, IEditableComponent<TagComponent>
{
    JAM_COMPONENT(TagComponent);

    // constructors
    TagComponent() = default;
    explicit TagComponent(std::string_view _name);

    // utilities
    NODISCARD bool operator==(const TagComponent& _other) const { return name == _other.name; }
    NODISCARD bool operator!=(const TagComponent& _other) const { return !(*this == _other); }
    NODISCARD bool operator==(const std::string_view _name) const { return name == _name; }
    NODISCARD bool operator!=(const std::string_view _name) const { return !(*this == _name); }

    // interface
    NODISCARD Json Serialize() const;
    void           Deserialize(const DeserializeParameter& _param);
    void           DrawEditor(const DrawEditorParameter& _param);

    std::string name = "unknown";
};

struct TransformComponent : ISerializableComponent<TransformComponent>, IEditableComponent<TransformComponent>
{
    JAM_COMPONENT(TransformComponent);

    // utilities
    NODISCARD Vec3 Up() const { return rotation * Vec3::UnitY; }
    NODISCARD Vec3 Down() const { return -Up(); }
    NODISCARD Vec3 Right() const { return rotation * Vec3::UnitX; }
    NODISCARD Vec3 Left() const { return -Right(); }
    NODISCARD Vec3 Forward() const { return rotation * Vec3::UnitZ; }
    NODISCARD Vec3 Backward() const { return -Forward(); }
    NODISCARD Mat4 CreateWorldMatrix() const;

    // interface
    NODISCARD Json Serialize() const;
    void           Deserialize(const DeserializeParameter& _param);
    void           DrawEditor(const DrawEditorParameter& _param);

    Vec3 position = Vec3::Zero;
    Quat rotation = Quat::Identity;
    Vec3 scale    = Vec3::One;
};

struct CameraComponent : ISerializableComponent<CameraComponent>, IEditableComponent<CameraComponent>
{
    JAM_COMPONENT(CameraComponent);

    // utilities
    NODISCARD Mat4 CreateViewMatrix(const Vec3& _position, const Vec3& _forward) const;
    NODISCARD Mat4 CreateProjectionMatrix() const;

    // interface
    NODISCARD Json Serialize() const;
    void           Deserialize(const DeserializeParameter& _param);
    void           DrawEditor(const DrawEditorParameter& _param);

    enum class eProjection
    {
        Perspective,
        Orthographic
    };

    float       fovY        = ToRad(45.f);
    float       nearZ       = 0.1f;
    float       farZ        = 1000.f;
    float       aspectRatio = 1.f;
    eProjection projection  = eProjection::Perspective;
};

struct ScriptComponent : ISerializableComponent<ScriptComponent>, IEditableComponent<ScriptComponent>
{
    JAM_COMPONENT(ScriptComponent)

    // constructors
    ScriptComponent() = default;
    explicit ScriptComponent(Scope<Script>&& _script);

    ~ScriptComponent()                                 = default;
    ScriptComponent(const ScriptComponent&)            = delete;
    ScriptComponent& operator=(const ScriptComponent&) = delete;
    ScriptComponent(ScriptComponent&&)                 = default;
    ScriptComponent& operator=(ScriptComponent&&)      = default;

    // overrides
    NODISCARD Json Serialize() const;
    void           Deserialize(const DeserializeParameter& _param);
    void           DrawEditor(const DrawEditorParameter& _param);

    Scope<Script> script;
};

struct ModelComponent : ISerializableComponent<ModelComponent>, IEditableComponent<ModelComponent>
{
    JAM_COMPONENT(ModelComponent);

    // serialization
    NODISCARD Json Serialize() const;
    void           Deserialize(const DeserializeParameter& _param);
    void           DrawEditor(const DrawEditorParameter& _param);

    Ref<ModelAsset> modelAsset;
};

}   // namespace jam