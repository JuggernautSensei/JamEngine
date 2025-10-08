#include "pch.h"

#include "Components.h"

#include "AssetManager.h"
#include "Scene.h"
#include "Script.h"
#include "ScriptDB.h"

namespace nlohmann
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Vec3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(jam::Quat, x, y, z, w);
}   // namespace nlohmann

namespace jam
{

Json TagComponent::Serialize(const Scene* _pScene, const Entity& _entity) const
{
    Json json;
    json["name"] = name;
    return json;
}

void TagComponent::Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity)
{
    name = _json.value<std::string>("name", "unknown");
}

Json TransformComponent::Serialize(const Scene* _pScene, const Entity& _entity) const
{
    JAM_ASSERT(root.IsValid(), "root entity is invalid");
    JAM_ASSERT(parent.IsValid(), "parent entity is invalid");
    JAM_ASSERT(owner.IsValid(), "owner entity is invalid");

    const Transform& local = transform.GetLocal();

    Json json;
    json["translate"] = local.translate;
    json["rotate"]    = local.rotate;
    json["scale"]     = local.scale;
    json["parent"]    = parent.GetUID();   // parent 만 저장하면 모두 복구할 수 있음
    return json;
}

void TransformComponent::Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity)
{
    JAM_ASSERT(root.IsValid(), "root entity is invalid");
    JAM_ASSERT(owner.IsValid(), "owner entity is invalid");

    Transform local;
    local.translate = _json.value<Vec3>("translate", Vec3::Zero);
    local.rotate    = _json.value<Quat>("rotate", Quat::Identity);
    local.scale     = _json.value<Vec3>("scale", Vec3::One);
    transform.SetLocal(local);

    if (_json.contains("parent"))
    {
        UID    parentUID    = _json["parent"].get<UID>();
        Entity parentEntity = _pScene->GetEntity(parentUID);
        JAM_ASSERT(parentEntity.IsValid(), "parent entity (UID: {}) not found", parentUID);
        SetParent(parentEntity);
    }
    else
    {
        // 부모 정보가 없으면 root에 붙임
        SetParent(root);
    }
}

void TransformComponent::SetParent(const Entity _parent)
{
    JAM_ASSERT(owner.IsValid(), "owner entity is invalid");
    JAM_ASSERT(owner != _parent, "cannot set self as parent");

    if (parent == _parent)
        return;

    // 기존 계층 정리
    if (parent.IsValid())
    {
        if (prevSibling.IsValid())
        {
            // 이전 형제와 다음 형제(혹은 nullptr)를 연결
            // nextSibling이 nullptr인 경우는 해당 노드가 마지막 자식인 경우
            TransformComponent& prevSiblingT = prevSibling.GetTransform();
            prevSiblingT.nextSibling         = nextSibling;
        }
        else
        {
            // 이전 형제가 없는 경우는 해당 노드가 첫 자식인 경우
            TransformComponent& parentT = parent.GetTransform();
            parentT.firstChild          = nextSibling;
        }

        if (nextSibling.IsValid())
        {
            // 다음 형제와 이전 형제(혹은 nullptr)를 연결
            // prevSibling이 nullptr인 경우는 해당 노드가 첫 자식인 경우
            TransformComponent& nextSiblingT = nextSibling.GetTransform();
            nextSiblingT.prevSibling         = prevSibling;
        }

        parent      = Entity::s_null;
        nextSibling = Entity::s_null;
        prevSibling = Entity::s_null;
    }

    // 새로운 부모 설정
    {
        parent                      = _parent;
        TransformComponent& parentT = parent.GetTransform();

        // 부모의 첫번째 자식으로 설정
        Entity parentFirstChild = parentT.firstChild;
        parentT.firstChild      = owner;
        nextSibling             = parentFirstChild;
        prevSibling             = Entity::s_null;
        if (parentFirstChild.IsValid())
        {
            TransformComponent& parentFirstChildT = parentFirstChild.GetTransform();
            parentFirstChildT.prevSibling         = owner;
        }

        // 계층 트랜스폼 갱신
        transform.SetParent(parentT.transform);
    }
}

void TransformComponent::ReleaseParent()
{
    JAM_ASSERT(owner.IsValid(), "owner entity is invalid");
    JAM_ASSERT(root.IsValid(), "cannot release parent of root entity");
    SetParent(root);
}

Json CameraComponent::Serialize(const Scene* _pScene, const Entity& _entity) const
{
    Json json;
    json["projection"] = camera.projection;
    json["nearZ"]      = camera.nearZ;
    json["farZ"]       = camera.farZ;
    json["fovYRad"]    = camera.fovYRad;
    json["width"]      = camera.width;
    json["height"]     = camera.height;
    json["bMain"]      = bMain;
    return json;
}

void CameraComponent::Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity)
{
    camera.projection = _json.value<eProjection>("projection", eProjection::Perspective);
    camera.nearZ      = _json.value<float>("nearZ", 0.1f);
    camera.farZ       = _json.value<float>("farZ", 1000.f);
    camera.fovYRad    = _json.value<float>("fovYRad", ToRad(45.f));
    camera.width      = _json.value<float>("width", 1.f);
    camera.height     = _json.value<float>("height", 1.f);
    bMain             = _json.value<bool>("bMain", false);

    if (!IsValidEnum(camera.projection))
    {
        camera.projection = eProjection::Perspective;
    }
}

Json ScriptComponent::Serialize(const Scene* _pScene, const Entity& _entity) const
{
    if (!pScript)
    {
        return {};
    }

    Json json;
    json["script"] = pScript->GetName();
    return json;
}

void ScriptComponent::Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity)
{
    if (_json.contains("script"))
    {
        std::string scriptName = _json["script"];
        pScript                = script_db::Create(scriptName, _entity);
    }
}

Json MeshRenderer::Serialize(const Scene* _pScene, const Entity& _entity) const
{
    Json json;

    if (mesh != k_invalidHandle)
    {
        const Mesh& m = asset_manager::GetMesh(mesh);
        json["mesh"]  = m.GetPath();
    }

    if (material != k_invalidHandle)
    {
        const Material& mat = asset_manager::GetMaterial(material);
        json["material"]    = mat.GetPath();
    }

    return json;
}

void MeshRenderer::Deserialize(const Json& _json, const Scene* _pScene, const Entity& _entity)
{

    if (_json.contains("mesh"))
    {
        fs::path path = _json["mesh"].get<fs::path>();
        auto [ret, m] = asset_manager::GetOrLoadMesh(path);
        if (IsOk(ret))
        {
            mesh = m;
        }
    }

    if (_json.contains("material"))
    {
        fs::path path   = _json["material"].get<fs::path>();
        auto [ret, mat] = asset_manager::GetOrLoadMaterial(path);
        if (IsOk(ret))
        {
            material = mat;
        }
    }
}

}   // namespace jam
