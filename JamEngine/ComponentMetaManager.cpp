#include "pch.h"

#include "ComponentMetaManager.h"

namespace
{

using namespace jam;

NODISCARD auto& GetMetaContainer()
{
    static ComponentMetaManager::MetaContainer s_container;
    return s_container;
}

NODISCARD auto& GetNameContainer()
{
    static ComponentMetaManager::NameContainer s_container;
    return s_container;
}

}   // namespace

namespace jam
{

void ComponentMetaManager::RegisterComponent(const ComponentMeta& _meta)
{
    // register component meta
    {
        auto& map        = ::GetMetaContainer();
        auto [_, result] = map.emplace(_meta.componentName, _meta);
        JAM_ASSERT(result, "Failed to register component '{}'", _meta.componentName);
    }

    // register component hash to name mapping
    {
        auto& nameMap    = GetNameContainer();
        auto [_, result] = nameMap.emplace(_meta.componentHash, _meta.componentName);
        JAM_ASSERT(result, "Failed to register component hash '{}' for component '{}'", _meta.componentHash, _meta.componentName);
    }
}

const std::unordered_map<std::string_view, ComponentMeta>& ComponentMetaManager::GetMetaContainer()
{
    return ::GetMetaContainer();
}

bool ComponentMetaManager::IsRegistered(const std::string_view _componentName)
{
    const auto& map = GetMetaContainer();
    return map.contains(_componentName);
}

std::string_view ComponentMetaManager::GetComponentNameByHash(UInt32 _componentHash)
{
    const auto& nameMap  = ::GetNameContainer();
    auto        it       = nameMap.find(_componentHash);
    bool        bIsExist = it != nameMap.end();

    if (bIsExist)   // 존재하면 이름을 리턴
    {
        return it->second;
    }
    else   // 존재하지 않으면 에러 로그 출력 + 빈 문자열 리턴
    {
        JAM_ERROR("Component with hash '{}' is not registered", _componentHash);
        return {};
    }
}

void ComponentMetaManager::CreateComponent(std::string_view _componentName, Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    const auto& map      = GetMetaContainer();
    auto        it       = map.find(_componentName);
    bool        bIsExist = it != map.end();

    // 메타 정보 가져오기
    if (bIsExist == false)
    {
        JAM_ERROR("Component '{}' is not registered", _componentName);
        return;
    }
    const ComponentMeta& meta = it->second;

    // 콜백이 설정되어 있는지 확인
    if (!meta.createComponentCallback)
    {
        JAM_ERROR("Create callback for component '{}' is not set", _componentName);
        return;
    }

    // 컴포넌트 생성
    meta.createComponentCallback(_owner);
}

void ComponentMetaManager::RemoveComponent(std::string_view _componentName, Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    const auto& map      = GetMetaContainer();
    auto        it       = map.find(_componentName);
    bool        bIsExist = it != map.end();

    // 메타 정보 가져오기
    if (bIsExist == false)
    {
        JAM_ERROR("Component '{}' is not registered", _componentName);
        return;
    }
    const ComponentMeta& meta = it->second;

    // 콜백이 설정되어 있는지 확인
    if (!meta.removeComponentCallback)
    {
        JAM_ERROR("Remove callback for component '{}' is not set", _componentName);
        return;
    }

    // 컴포넌트 제거
    meta.removeComponentCallback(_owner);
}

void* ComponentMetaManager::GetComponentOrNull(std::string_view _componentName, const Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    const auto& map      = GetMetaContainer();
    auto        it       = map.find(_componentName);
    bool        bIsExist = it != map.end();

    // 메타 정보 가져오기
    if (bIsExist == false)
    {
        JAM_ERROR("Component '{}' is not registered", _componentName);
        return nullptr;
    }
    const ComponentMeta& meta = it->second;

    // 콜백이 설정되어 있는지 확인
    if (!meta.getComponentCallback)
    {
        JAM_ERROR("GetComponentOrNull callback for component '{}' is not set", _componentName);
        return nullptr;
    }

    // 컴포넌트 포인터 반환
    return meta.getComponentCallback(_owner);
}

bool ComponentMetaManager::HasComponent(std::string_view _componentName, const Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    const auto& map      = GetMetaContainer();
    auto        it       = map.find(_componentName);
    bool        bIsExist = it != map.end();

    // 메타 정보 가져오기
    if (bIsExist == false)
    {
        JAM_ERROR("Component '{}' is not registered", _componentName);
        return false;
    }
    const ComponentMeta& meta = it->second;

    // 콜백이 설정되어 있는지 확인
    if (!meta.hasComponentCallback)
    {
        JAM_ERROR("Has callback for component '{}' is not set", _componentName);
        return false;
    }

    // 컴포넌트 존재 여부 확인
    return meta.hasComponentCallback(_owner);
}

Json ComponentMetaManager::SerializeComponent(std::string_view _componentName, const Entity& _owner)
{
    JAM_ASSERT(_owner.IsValid(), "Owner entity is not valid");
    const auto& map      = GetMetaContainer();
    auto        it       = map.find(_componentName);
    bool        bIsExist = it != map.end();

    // 메타 정보 가져오기
    if (bIsExist == false)
    {
        JAM_ERROR("Component '{}' is not registered", _componentName);
        return {};
    }
    const ComponentMeta& meta = it->second;

    // 콜백이 설정되어 있는지 확인
    if (!(meta.serializeComponentCallback && meta.getComponentCallback))
    {
        JAM_ERROR("Serialize and getter callback for component '{}' is not set", _componentName);
        return {};
    }

    void* pComponentValue = meta.getComponentCallback(_owner);
    if (!pComponentValue)
    {
        JAM_ERROR("Component '{}' is not attached to owner entity or serialize impossible component. may be empty struct", _componentName);
        return {};
    }

    // 컴포넌트 직렬화
    return meta.serializeComponentCallback(pComponentValue);
}

void ComponentMetaManager::DeserializeComponent(std::string_view _componentName, const Json& _json, Scene* _pScene, const Entity& _onwerEntity)
{
    JAM_ASSERT(_pScene, "DeserializeParameter::pScene must not be nullptr");

    const auto& map      = GetMetaContainer();
    auto        it       = map.find(_componentName);
    bool        bIsExist = it != map.end();

    // 메타 정보 가져오기
    if (bIsExist == false)
    {
        JAM_ERROR("Component '{}' is not registered", _componentName);
        return;
    }
    const ComponentMeta& meta = it->second;

    // 콜백이 설정되어 있는지 확인
    if (!(meta.serializeComponentCallback && meta.getComponentCallback))
    {
        JAM_ERROR("Serialize and getter callback for component '{}' is not set", _componentName);
        return;
    }

    void* pComponentValue = meta.getComponentCallback(_onwerEntity);
    if (!pComponentValue)
    {
        JAM_ERROR("Component '{}' is not attached to owner entity or deserialize impossible component. may be empty struct", _componentName);
        return;
    }

    // 컴포넌트 역직렬화
    meta.deserializeComponentCallback(_json, _pScene, _onwerEntity, pComponentValue);
}

void ComponentMetaManager::DrawComponentEditor(std::string_view _componentName, EditorLayer* const _pEditorLayer, Scene* _pScene, const Entity& _ownerEntity)
{
    JAM_ASSERT(_pEditorLayer, "OnEditorParameter::pEditorLayer must not be nullptr");

    auto& map      = GetMetaContainer();
    auto  it       = map.find(_componentName);
    bool  bIsExist = it != map.end();

    // 메타 정보 가져오기
    if (bIsExist == false)
    {
        JAM_ERROR("Component '{}' is not registered", _componentName);
        return;
    }
    const ComponentMeta& meta = it->second;

    // 콜백이 설정되어 있는지 확인
    if (!(meta.serializeComponentCallback && meta.getComponentCallback))
    {
        JAM_ERROR("Serialize and getter callback for component '{}' is not set", _componentName);
        return;
    }

    void* pComponentValue = meta.getComponentCallback(_ownerEntity);
    if (!pComponentValue)
    {
        JAM_ERROR("Component '{}' is not attached to owner entity or draw edit impossible component. may be empty struct", _componentName);
        return;
    }

    // 컴포넌트 에디터 그리기
    meta.drawComponentEditorCallback(_pEditorLayer, _pScene, _ownerEntity, pComponentValue);
}

}   // namespace jam