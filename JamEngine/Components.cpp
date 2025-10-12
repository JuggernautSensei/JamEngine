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

Json TagComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    Json json;
    json["name"] = name;
    return json;
}

eErrorCode TagComponent::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    // 유효성 체크
    if (!_json.contains("name"))
    {
        SetErrorString("TagComponent deserialization failed: 'name' field is missing.");
        return eErrorCode::FileParseFailed;
    }

    name = _json["name"].get<std::string>();
    return eErrorCode::Ok;
}

Json TransformComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    JAM_ASSERT(root.IsValid(), "root entity is invalid");
    JAM_ASSERT(parent.IsValid(), "parent entity is invalid");
    JAM_ASSERT(owner.IsValid(), "owner entity is invalid");

    const Transform& local = transform.GetLocal();

    Json json;
    json["translate"] = local.translate;
    json["rotate"]    = local.rotate;
    json["scale"]     = local.scale;
    json["parent"]    = parent.GetUID();
    return json;
}

eErrorCode TransformComponent::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    JAM_ASSERT(root.IsValid(), "root entity is invalid");
    JAM_ASSERT(owner.IsValid(), "owner entity is invalid");

    // 유효성 체크
    const char* fieldNames[] = { "translate", "rotate", "scale", "parent" };
    for (const char* fieldName: fieldNames)
    {
        if (!_json.contains(fieldName))
        {
            SetErrorString("TransformComponent deserialization failed: {} field is missing.", fieldName);
            return eErrorCode::FileParseFailed;
        }
    }

    // 로컬
    Transform local;
    local.translate = _json.value<Vec3>("translate", Vec3::Zero);
    local.rotate    = _json.value<Quat>("rotate", Quat::Identity);
    local.scale     = _json.value<Vec3>("scale", Vec3::One);
    transform.SetLocal(local);

    // 부모 설정
    UID    parentUID = _json["parent"].get<UID>();
    Entity parent    = _pScene->FindEntity(parentUID);
    if (!parent.IsValid())
    {
        SetErrorString("TransformComponent deserialization failed: 'parent' entity with UID " + std::to_string(parentUID) + " not found.");
        return eErrorCode::FileParseFailed;
    }

    SetParent(parent);
    return eErrorCode::Ok;
}

void TransformComponent::SetParent(const Entity _parent)
{
    JAM_ASSERT(owner.IsValid(), "owner entity is invalid");
    JAM_ASSERT(owner != _parent, "cannot set self as parent");

    if (parent == _parent)
    {
        return;
    }

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

Json CameraComponent::Serialize(const Scene* _pScene, Entity _entity) const
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

eErrorCode CameraComponent::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    constexpr const char* const fieldNames[] = { "projection", "nearZ", "farZ", "fovYRad", "width", "height", "bMain" };
    for (const char* fieldName: fieldNames)
    {
        if (!_json.contains(fieldName))
        {
            SetErrorString("CameraComponent deserialization failed: {} field is missing.", fieldName);
            return eErrorCode::FileParseFailed;
        }
    }

    camera.projection = _json["projectection"].get<eProjection>();
    camera.nearZ      = _json["nearZ"].get<float>();
    camera.farZ       = _json["farZ"].get<float>();
    camera.fovYRad    = _json["fovYRad"].get<float>();
    camera.width      = _json["width"].get<float>();
    camera.height     = _json["height"].get<float>();
    bMain             = _json["bMain"].get<bool>();

    // 사후검사
    if (!IsValidEnum(camera.projection))
    {
        SetErrorString("CameraComponent deserialization failed: 'projection' field is invalid.");
        return eErrorCode::FileParseFailed;
    }

    camera.nearZ = Clamp(camera.nearZ, Camera::k_nearestZ, Camera::k_farthestZ - Camera::k_nearestZ);
    camera.farZ  = Clamp(camera.farZ, camera.nearZ + Camera::k_nearestZ, Camera::k_farthestZ);
    return eErrorCode::Ok;
}

Json ScriptComponent::Serialize(const Scene* _pScene, Entity _entity) const
{
    if (pScript)
    {
        Json json;
        json["script"] = pScript->GetName();
        return json;
    }
    else
    {
        Json json;
        json["script"] = nullptr;
        return json;
    }
}

eErrorCode ScriptComponent::Deserialize(const Json& _json, const Scene* _pScene, const Entity _entity)
{
    if (!_json.contains("script"))
    {
        SetErrorString("ScriptComponent deserialization failed: 'script' field is missing.");
        return eErrorCode::FileParseFailed;
    }

    Json scriptField = _json["script"];
    if (scriptField.is_null())
    {
        return eErrorCode::Ok;   // 스크립트 없음
    }
    else if (scriptField.is_string())
    {
        std::string scriptName = scriptField.get<std::string>();
        pScript                = script_db::Create(scriptName, _entity);
        return eErrorCode::Ok;
    }

    SetErrorString("ScriptComponent deserialization failed: 'script' field is not a string or null.");
    return eErrorCode::FileParseFailed;
}

Json MeshRenderer::Serialize(const Scene* _pScene, Entity _entity) const
{
    Json json;

    // mesh field
    if (mesh)
    {
        json["mesh"] = mesh->GetPath();
    }
    else
    {
        json["mesh"] = nullptr;
    }

    // material field
    if (material)
    {
        json["material"] = material->GetPath();
    }
    else
    {
        json["material"] = nullptr;
    }

    return json;
}
eErrorCode MeshRenderer::Deserialize(const Json& _json, const Scene* _pScene, Entity _entity)
{
    constexpr const char* const fieldNames[] = { "mesh", "material" };
    for (const char* fieldName: fieldNames)
    {
        if (!_json.contains(fieldName))
        {
            SetErrorString("MeshRenderer deserialization failed: {} field is missing.", fieldName);
            return eErrorCode::FileParseFailed;
        }
    }

    // mesh field
    Json meshField = _json["mesh"];
    if (meshField.is_null())
    {
        mesh = nullptr;
    }
    else if (meshField.is_string())
    {
        std::string meshPath  = meshField.get<std::string>();
        auto [ec, meshHandle] = asset_manager::GetOrLoadMesh(meshPath);

        if (IsError(ec))
            return ec;

        mesh = meshHandle;
    }
    else
    {
        SetErrorString("MeshRenderer deserialization failed: 'mesh' field is not a string or null.");
        return eErrorCode::FileParseFailed;
    }

    // material field
    Json materialField = _json["material"];
    if (materialField.is_null())
    {
        material = nullptr;
    }
    else if (materialField.is_string())
    {
        std::string materialPath  = materialField.get<std::string>();
        auto [ec, materialHandle] = asset_manager::GetOrLoadMaterial(materialPath);

        if (IsError(ec))
            return ec;

        material = materialHandle;
    }
    else
    {
        SetErrorString("MeshRenderer deserialization failed: 'material' field is not a string or null.");
        return eErrorCode::FileParseFailed;
    }
    return eErrorCode::Ok;
}

}   // namespace jam
