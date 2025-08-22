#include "pch.h"

#include "Scene.h"

#include "Application.h"
#include "Components.h"
#include "Config.h"
#include "Entity.h"
#include "SceneSerializer.h"
#include "ShaderBridge.h"

namespace jam
{

Scene::Scene(const std::string_view _name)
    : m_name(_name)
{
}

void Scene::OnEnter()
{
}

bool Scene::Save(const std::optional<fs::path>& _path)
{
    fs::path path;
    if (_path)
    {
        path = *_path;   // 경로가 주어지면 해당 경로 사용
    }
    else
    {
        path = GetApplication().GetScenesDirectory() / m_name;
        path.replace_extension(k_jamSceneExtensionW);   // 확장자를 jam으로 변경
    }

    SceneSerializer serializer;
    serializer.Serialize(this);          // 현재 씬을 직렬화
    if (serializer.SaveFromFile(path))   // 저장 성공
    {
        Log::Info("Scene saved to: {}", path.string());
        return true;   // 성공적으로 저장되면 true 반환
    }
    else
    {
        JAM_ERROR("Failed to save scene to: {}", path.string());
        return false;   // 저장 실패 시 false 반환
    }
}

bool Scene::Load(const std::optional<fs::path>& _path)
{
    fs::path path;
    if (_path)
    {
        path = *_path;   // 경로가 주어지면 해당 경로 사용
    }
    else
    {
        path = GetApplication().GetScenesDirectory() / m_name;
        path.replace_extension(k_jamSceneExtensionW);   // 확장자를 jam으로 변경
    }

    // 만약 새롭게 만든 씬이라면 경로가 존재하지 않을 수 있음.
    if (!fs::exists(path))
    {
        Log::Debug("Scene::Load() - Scene file does not exist: {}", path.string());
        return false;   // 경로가 존재하지 않으면 false 반환
    }

    // 씬 직렬화 로드
    SceneSerializer serializer;
    if (serializer.LoadFromFile(path))   // 로드 성공
    {
        serializer.Deserialize(this);   // 직렬화된 씬을 현재 씬에 적용
        Log::Info("Scene loaded from: {}", path.string());
        return true;
    }
    else
    {
        JAM_ERROR("Failed to load scene from: {}", path.string());
        return false;
    }
}

void Scene::Clear()
{
    ClearEntities();
    m_assetManager.ClearAll();
}

Entity Scene::CreateEntity()
{
    entt::entity handle = m_registry.create();
    Entity       entity { this, handle };
    entity.CreateComponent<TransformComponent>();   // 기본적으로 TransformComponent를 생성
    return entity;
}

Entity Scene::CreateEntity(UInt32 _id)
{
    entt::entity handle = m_registry.create(static_cast<entt::entity>(_id));
    Entity       entity { this, handle };
    entity.CreateComponent<TransformComponent>();   // 기본적으로 TransformComponent를 생성
    return entity;
}

Entity Scene::GetEntity(const entt::entity _handle)
{
    if (m_registry.valid(_handle))
    {
        return Entity { this, _handle };
    }
    else
    {
        return Entity::s_null;   // 유효하지 않은 핸들인 경우 null 엔티티 반환
    }
}

Entity Scene::GetEntity(const UInt32 _id)
{
    return GetEntity(static_cast<entt::entity>(_id));
}

Entity Scene::CloneEntity(const entt::entity _handle)
{
    const Entity       clonedEntity = CreateEntity();
    const entt::entity clonedHandle = clonedEntity.GetHandle();

    for (auto&& [id, set]: m_registry.storage())
    {
        if (!set.contains(clonedHandle) && set.contains(_handle))
        {
            const void* elem = set.value(_handle);
            set.push(clonedHandle, elem);
        }
    }
    return clonedEntity;
}

void Scene::DestroyEntity(const entt::entity _handle)
{
    // 바로 파괴하면 현 프레임에서 문제가 발생할 수 있음
    // 그러므로 삭제를 예약
    // 프레임이 끝난 후 배치처리
    GetApplication().SubmitCommand(
        [this, _handle]
        {
            m_registry.destroy(_handle);
        });
}

}   // namespace jam