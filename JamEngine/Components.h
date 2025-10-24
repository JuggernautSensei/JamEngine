#pragma once
#include "Camera.h"
#include "ComponentDB.h"
#include "TransformHierarchy.h"
#include "Material.h"
#include "Mesh.h"

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

struct CameraComponent : SerializableComponent<CameraComponent>
{
    JAM_COMPONENT(CameraComponent);

    JAM_NODISCARD Json Serialize(const Scene* _pScene, Entity _entity) const;
    eErrorCode         Deserialize(const Json& _json, const Scene* _pScene, Entity _entity);

    Camera camera = {};
    bool   bMain  = false;
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

}   // namespace jam