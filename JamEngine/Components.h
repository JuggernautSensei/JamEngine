#pragma once
#include "ComponentDB.h"
#include "Material.h"
#include "Mesh.h"
#include "TransformHierarchy.h"

// clang-format off
namespace jam { class Script; }
// clang-format on

namespace jam
{

struct TagComponent : SerializableComponent<TagComponent>
{
    JAM_COMPONENT(TagComponent);

    JAM_NODISCARD bool operator==(const TagComponent& _other) const { return name == _other.name; }
    JAM_NODISCARD bool operator!=(const TagComponent& _other) const { return !(*this == _other); }
    JAM_NODISCARD bool operator==(const std::string_view _name) const { return name == _name; }
    JAM_NODISCARD bool operator!=(const std::string_view _name) const { return !(*this == _name); }

    JAM_NODISCARD Json Serialize(const Scene* _pScene, Entity _entity) const;
    eErrorCode         Deserialize(const Json& _json, const Scene* _pScene, Entity _entity);

    std::string name = "unknown";
};

struct TransformComponent : UnremovableComponent<TransformComponent>, SerializableComponent<TransformComponent>
{
    JAM_COMPONENT(TransformComponent);

    JAM_NODISCARD Json Serialize(const Scene* _pScene, Entity _entity) const;
    eErrorCode         Deserialize(const Json& _json, const Scene* _pScene, Entity _entity);

    TransformHierarchy transform = {};
};

enum class eProjection : Int8
{
    Perspective = 0,
    Orthographic
};

struct CameraComponent : SerializableComponent<CameraComponent>
{
    JAM_COMPONENT(CameraComponent);

    JAM_NODISCARD Json Serialize(const Scene* _pScene, Entity _entity) const;
    eErrorCode         Deserialize(const Json& _json, const Scene* _pScene, Entity _entity);

    JAM_NODISCARD Mat4 CreateProj() const;

    eProjection projection = eProjection::Perspective;
    float       nearZ      = 0.1f;
    float       farZ       = 1000.f;
    float       fovYRad    = ToRad(45.f);
    float       width      = 1.f;
    float       height     = 1.f;
    bool        bMain      = false;

    static constexpr float k_nearestZ  = 0.01f;
    static constexpr float k_farthestZ = 10000.f;
};

struct ScriptComponent : SerializableComponent<ScriptComponent>
{
    JAM_COMPONENT(ScriptComponent);

    template<typename T>
    void BindScript(Entity _entity)
    {
        static_assert(T::_script_marker_ && std::is_base_of_v<Script, T>, "Script must use JAM_SCRIPT macro and be derived from Script class");
        pScript = std::make_shared<T>(_entity);
    }

    JAM_NODISCARD Json Serialize(const Scene* _pScene, Entity _entity) const;
    eErrorCode         Deserialize(const Json& _json, const Scene* _pScene, Entity _entity);

    std::shared_ptr<Script> pScript;
};

struct MeshRenderer : SerializableComponent<MeshRenderer>
{
    JAM_COMPONENT(MeshRenderer);

    JAM_NODISCARD Json Serialize(const Scene* _pScene, Entity _entity) const;
    eErrorCode         Deserialize(const Json& _json, const Scene* _pScene, Entity _entity);

    std::shared_ptr<Mesh>     pMesh     = nullptr;
    std::shared_ptr<Material> pMaterial = nullptr;
};

struct PointLightComponent : SerializableComponent<PointLightComponent>
{
    JAM_COMPONENT(PointLightComponent);
    JAM_NODISCARD Json Serialize(const Scene* _pScene, Entity _entity) const;
    eErrorCode         Deserialize(const Json& _json, const Scene* _pScene, Entity _entity);

    Vec3  radiance     = Vec3::Zero;
    float intensity    = 1.0f;
    float fallOffStart = 1.0f;
    float fallOffEnd   = 10.0f;
};

struct SpotLightComponent : SerializableComponent<SpotLightComponent>
{
    JAM_COMPONENT(SpotLightComponent);
    JAM_NODISCARD Json Serialize(const Scene* _pScene, Entity _entity) const;
    eErrorCode         Deserialize(const Json& _json, const Scene* _pScene, Entity _entity);

    Vec3  radiance         = Vec3::Zero;
    float intensity        = 1.f;
    float fallOffStart     = 1.f;
    float fallOffEnd       = 10.f;
    float innerConeHalfRad = ToRad(15.f);
    float outerConeHalfRad = ToRad(30.f);
};

struct ShadowCasterComponent : SerializableComponent<ShadowCasterComponent>
{
    JAM_COMPONENT(ShadowCasterComponent);
    JAM_NODISCARD Json Serialize(const Scene* _pScene, Entity _entity) const;
    eErrorCode         Deserialize(const Json& _json, const Scene* _pScene, Entity _entity) const;
};

}   // namespace jam