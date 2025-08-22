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
    void           Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity);
    void           DrawEditor(EditorLayer* _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity);

    std::string name = "unknown";
};

struct TransformComponent : ISerializableComponent<TransformComponent>, IEditableComponent<TransformComponent>
{
    JAM_COMPONENT(TransformComponent);

    // utilities
    NODISCARD Vec3 Up() const { return Vec3::Transform(Vec3::UnitY, rotation); }
    NODISCARD Vec3 Down() const { return -Up(); }
    NODISCARD Vec3 Right() const { return Vec3::Transform(Vec3::UnitX, rotation); }
    NODISCARD Vec3 Left() const { return -Right(); }
    NODISCARD Vec3 Forward() const { return Vec3::Transform(Vec3::UnitZ, rotation); }
    NODISCARD Vec3 Backward() const { return -Forward(); }
    NODISCARD Mat4 CreateWorldMatrix() const;

    // interface
    NODISCARD Json Serialize() const;
    void           Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity);
    void           DrawEditor(EditorLayer* _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity);

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
    void           Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity);
    void           DrawEditor(EditorLayer* _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity);

    enum class eProjection
    {
        Perspective,
        Orthographic
    };

    static constexpr float k_nearestZ  = 0.01f;
    static constexpr float k_farthestZ = 10000.f;

    float       fovYRad     = ToRad(45.f);                // 수직 시야갹 (라디안)
    float       nearZ       = 0.1f;                       // 근평면
    float       farZ        = 1000.f;                     // 원평면
    float       aspectRatio = 1.f;                        // 카메라의 종횡비
    eProjection projection  = eProjection::Perspective;   // 투영 방식
    bool        bPrimary    = false;                      // 메인 카메라 여부
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
    void           Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity);
    void           DrawEditor(EditorLayer* _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity);

    Scope<Script> script;
};

class ModelAsset;
struct ModelComponent : ISerializableComponent<ModelComponent>, IEditableComponent<ModelComponent>
{
    JAM_COMPONENT(ModelComponent);

    // serialization
    NODISCARD Json Serialize() const;
    void           Deserialize(const Json& _json, Scene* _pScene, const Entity& _onwerEntity);
    void           DrawEditor(EditorLayer* _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity);

    Ref<ModelAsset> modelAsset;
};

}   // namespace jam