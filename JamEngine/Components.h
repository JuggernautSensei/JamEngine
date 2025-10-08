#pragma once
#include "Camera.h"
#include "ComponentDB.h"
#include "HierarchyTransform.h"
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

    JAM_NODISCARD Json Serialize(const Scene* _pScene, const Entity& _entity) const;
    void               Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity);

    std::string name = "unknown";
};

struct TransformComponent : UnremovableComponent<TransformComponent>, SerializableComponent<TransformComponent>
{
    JAM_COMPONENT(TransformComponent);

    JAM_NODISCARD Json Serialize(const Scene* _pScene, const Entity& _entity) const;
    void               Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity);

    void SetParent(Entity _parent);
    void ReleaseParent();

    HierarchyTransform transform = {};

    Entity root        = Entity::s_null;
    Entity owner       = Entity::s_null;
    Entity parent      = Entity::s_null;
    Entity firstChild  = Entity::s_null;
    Entity nextSibling = Entity::s_null;
    Entity prevSibling = Entity::s_null;
};

struct CameraComponent : SerializableComponent<CameraComponent>
{
    JAM_COMPONENT(CameraComponent);

    JAM_NODISCARD Json Serialize(const Scene* _pScene, const Entity& _entity) const;
    void               Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity);

    Camera camera = {};
    bool   bMain  = false;
};

struct ScriptComponent : SerializableComponent<ScriptComponent>
{
    JAM_COMPONENT(ScriptComponent);

    JAM_NODISCARD Json Serialize(const Scene* _pScene, const Entity& _entity) const;
    void               Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity);

    std::unique_ptr<Script> pScript;
};

struct MeshRenderer : SerializableComponent<MeshRenderer>
{
    JAM_COMPONENT(MeshRenderer);

    JAM_NODISCARD Json Serialize(const Scene* _pScene, const Entity& _entity) const;
    void               Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity);

    MeshHandle     mesh     = k_invalidHandle;
    MaterialHandle material = k_invalidHandle;
};

}   // namespace jam