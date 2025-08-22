#include "pch.h"

#include "AssetManager.h"

#include "Application.h"
#include "ModelAsset.h"
#include "TextureAsset.h"

namespace
{

using namespace jam;

// 올바른 키는 work directory 로 부터의 상대 경로
NODISCARD Result<fs::path> CreateKeyFromPath(const fs::path& _path)
{
    // 프로젝트 경로를 기준으로 키 생성
    fs::path          key     = fs::relative(_path, GetApplication().GetWorkingDirectory());
    std::wstring_view keyWStr = key.native();

    if (keyWStr.starts_with(L"..") == false && keyWStr.empty() == false)   // 정확한 상대경로인지 확인
    {
        return key;
    }
    else
    {
        return Fail;
    }
}

}   // namespace

namespace jam
{

void AssetManager::Clear(const eAssetType _type)
{
    JAM_ASSERT(IsValidEnum(_type), "AssetManager::Reset() - Invalid asset type");
    m_containers[EnumToInt(_type)].clear();
}

void AssetManager::ClearAll()
{
    for (Container& container: m_containers)
    {
        container.clear();   // Reset each asset type container
    }
}

Result<Ref<Asset>> AssetManager::GetOrLoad(const eAssetType _type, const fs::path& _path)
{
    Container& container = GetContainer_(_type);
    auto       it        = container.find(_path);

    if (it != container.end())   // 찾았을 경우 기존 에셋 리턴
    {
        return it->second;
    }
    else   // 찾지 못했을 경우 로드
    {
        return Load(_type, _path);
    }
}

Result<Ref<Asset>> AssetManager::Load(const eAssetType _type, const fs::path& _path)
{
    auto [key, bResult] = CreateKeyFromPath(_path);   // 키 생성
    if (!bResult)                                     // invalid path
    {
        JAM_ERROR("AssetManager::Load() - Invalid asset path: {}", _path.string());
        return Fail;
    }

    Container& container = GetContainer_(_type);          // 타입 컨테이너
    auto       iterator  = container.find(key);           // 키로 컨테이너에서 찾기
    bool       bExists   = iterator != container.end();   // 키가 이미 존재하는지 확인
    Ref<Asset> pAsset    = bExists ? iterator->second : CreateAsset_(_type);

    // 로드 (만약 이미 존재하는 에셋이라면 덮어쓴다.)
    if (!pAsset->Load(*this, key))
    {
        // 로드 실패
        JAM_ERROR("AssetManager::Load() - Failed to load asset from path: {}", _path.string());
        return Fail;
    }

    // 로드 완료 이벤트 전송
    if (bExists)
    {
        AssetModifiedEvent event(_type, _path);   // 수정 이벤트 전송
        GetApplication().DispatchEvent(event);
    }
    else   // 존재하지 않음 - 새로 생성 이벤트 전송 + 컨테이너에 추가
    {
        container[key] = pAsset;              // 새로운 에셋을 컨테이너에 추가
        AssetLoadEvent event(_type, _path);   // 생성 이벤트 전송
        GetApplication().DispatchEvent(event);
    }
    return pAsset;   // Return the loaded or existing asset
}

bool AssetManager::Unload(const eAssetType _type, const fs::path& _path)
{
    auto [key, bResult] = CreateKeyFromPath(_path);   // 키 생성
    if (bResult == false)                             // invalid path
    {
        JAM_ERROR("AssetManager::Reset() - Invalid asset path: {}", _path.string());
        return false;   // Invalid path
    }

    Container& container = GetContainer_(_type);          // 타입 컨테이너
    auto       iterator  = container.find(key);           // 에셋 탐색
    bool       bExists   = iterator != container.end();   // 키가 존재하는지 확인

    if (bExists == false)   // 발견하지 못함
    {
        JAM_ERROR("AssetManager::Reset() - Asset not found at path: {}", _path.string());
        return false;
    }

    // 언로드
    const Ref<Asset>& pAsset = iterator->second;
    pAsset->Unload();

    // 컨테이너에서 제거
    container.erase(iterator);

    // 제거 이벤트 전송
    AssetUnloadEvent event(_type, _path);
    GetApplication().DispatchEvent(event);
    return true;   // 제거 성공
}

bool AssetManager::Contain(const eAssetType _type, const fs::path& _path) const
{
    auto [key, bResult] = CreateKeyFromPath(_path);   // 키 생성
    if (bResult == false)
    {
        return false;
    }

    // 키가 존재하는지 탐색
    const Container& container = GetContainer_(_type);
    return container.contains(key);
}

Result<Ref<Asset>> AssetManager::Get(const eAssetType _type, const fs::path& _path) const
{
    auto [key, bResult] = CreateKeyFromPath(_path);   // 키 생성
    if (bResult == false)                             // invalid path
    {
        JAM_ERROR("AssetManager::Get() - Invalid asset path: {}", _path.string());
        return Fail;
    }

    // 키가 올바른 경우
    const Container& container = GetContainer_(_type);   // 타입 컨테이너
    auto             iterator  = container.find(key);
    if (iterator != container.end())   // 찾았을 경우
    {
        return iterator->second;   // 에셋 리턴
    }
    else   // 찾지 못했을 경우
    {
        JAM_ERROR("AssetManager::Get() - Asset not found at path: {}", _path.string());
        return Fail;   // 실패 반환
    }
}

Ref<Asset> AssetManager::CreateAsset_(const eAssetType _type) const
{
    switch (_type)
    {
        case eAssetType::Model: return MakeRef<ModelAsset>();
        case eAssetType::Texture: return MakeRef<TextureAsset>();
    }
    JAM_CRASH("Unsupported asset type: {}", EnumToInt(_type));
}

AssetManager::Container& AssetManager::GetContainer_(const eAssetType _type)
{
    return m_containers[EnumToInt(_type)];
}

const AssetManager::Container& AssetManager::GetContainer_(const eAssetType _type) const
{
    return m_containers[EnumToInt(_type)];
}

}   // namespace jam