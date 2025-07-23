#pragma once

namespace jam
{
class Script;

struct TagComponent
{
    TagComponent() = default;
    explicit TagComponent(std::string_view _name);

    NODISCARD bool operator==(const TagComponent& _other) const { return name == _other.name; }
    NODISCARD bool operator!=(const TagComponent& _other) const { return !(*this == _other); }
    NODISCARD bool operator==(const std::string_view _name) const { return name == _name; }
    NODISCARD bool operator!=(const std::string_view _name) const { return !(*this == _name); }

    std::string name;
};

struct TransformComponent
{
    Vec3 position = Vec3::Zero;
    Quat rotation = Quat::Identity;
    Vec3 scale    = Vec3::One;
};

struct CameraComponent
{
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

struct ScriptComponent
{
    ScriptComponent() = default;
    explicit ScriptComponent(std::unique_ptr<Script>&& _script);

    std::unique_ptr<Script> script;
};

}   // namespace jam