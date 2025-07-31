#pragma once
#include "ComponentMetaManager.h"
#include "Script.h"

namespace jam
{

struct TagComponent : public IComponentSerializable<TagComponent>
{
    JAM_COMPONENT(TagComponent);

    // constructors
    TagComponent() = default;
    explicit TagComponent(std::string_view _name);

    // comparison operators
    NODISCARD bool operator==(const TagComponent& _other) const { return name == _other.name; }
    NODISCARD bool operator!=(const TagComponent& _other) const { return !(*this == _other); }
    NODISCARD bool operator==(const std::string_view _name) const { return name == _name; }
    NODISCARD bool operator!=(const std::string_view _name) const { return !(*this == _name); }

    // serialization
    NODISCARD Json Serialize() const;
    void           Deserialize(const ComponentDeserializeData& _deserializeData);

    std::string name;
};

struct TransformComponent : public IComponentSerializable<TransformComponent>
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

    // serialization
    NODISCARD Json Serialize() const;
    void           Deserialize(const ComponentDeserializeData& _deserializeData);

    Vec3 position = Vec3::Zero;
    Quat rotation = Quat::Identity;
    Vec3 scale    = Vec3::One;
};

struct CameraComponent : public IComponentSerializable<CameraComponent>
{
    JAM_COMPONENT(CameraComponent);

    // serialization
    NODISCARD Json Serialize() const;
    void           Deserialize(const ComponentDeserializeData& _deserializeData);

    enum class eProjection
    {
        Perspective,
        Orthographic,
    };

    float       fovY        = 45.f;
    float       nearZ       = 0.1f;
    float       farZ        = 1000.f;
    float       aspectRatio = 1.f;
    eProjection projection  = eProjection::Perspective;
};

struct ScriptComponent : public IComponentSerializable<ScriptComponent>
{
    JAM_COMPONENT(ScriptComponent)

    // constructors
    ScriptComponent() = default;
    explicit ScriptComponent(std::unique_ptr<Script>&& _script);

    ~ScriptComponent()                                 = default;
    ScriptComponent(const ScriptComponent&)            = delete;
    ScriptComponent& operator=(const ScriptComponent&) = delete;
    ScriptComponent(ScriptComponent&&)                 = default;
    ScriptComponent& operator=(ScriptComponent&&)      = default;

    // serialization
    NODISCARD Json Serialize() const;
    void           Deserialize(const ComponentDeserializeData& _deserializeData);

    std::unique_ptr<Script> script;
};

}   // namespace jam