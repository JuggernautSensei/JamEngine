#include "pch.h"
#include "ResourceManager.h"

void ResourceManager::OnUpdate(
    const float _dt)
{
    m_garbageCollectCounterSec += _dt;
    if (m_garbageCollectCounterSec >= k_garbageCollectInterval)
    {
        CleanUpGarbage();
        m_garbageCollectCounterSec = 0.f;
    }
}

Expect<Ref<ShaderSource>> ResourceManager::LoadShaderSource(
    const StringView _path,
    const bool       _bReload)
{
    const auto iter = m_shaderSources.find(_path);
    if (iter != m_shaderSources.end())
    {
        Ref<ShaderSource>& pSource = iter->second;

        if (_bReload)
        {
            if (const Error err = pSource->Reload())
            {
                return err;
            }

            return pSource;
        }

        return pSource;
    }

    // if not found, load new shader source
    Expect<Ref<ShaderSource>> pSource = ShaderSource::Create(_path);
    if (!pSource)
    {
        return pSource.GetError();
    }

    const auto [it, bRet] = m_shaderSources.emplace(_path, pSource.Take());
    JAM_ASSERT(bRet, "Failed to insert shader source into the map. This should never happen since we already checked for existence.");
    return it->second;
}

bool ResourceManager::HasShaderSource(
    const StringView _path) const
{
    return m_shaderSources.contains(_path);
}

void ResourceManager::ConnectResourceOnChange(
    const Ref<Resource>&    _pSource,
    const OnChangeDelegate& _delegate,
    const Ref<Resource>&    _pTarget)
{
    JAM_ASSERT(_pSource, "Cannot connect change for a null source resource.");
    JAM_ASSERT(_delegate, "Cannot connect change for an empty delegate.");
    JAM_ASSERT(_pTarget, "Cannot connect change for a null target resource.");
    JAM_ASSERT(_delegate.GetCallerAddress() == reinterpret_cast<UInt64>(_pSource.GetPtr()), "Delegate caller does not match the source resource.");

    ResourceData& targetData = m_resourceData[_pTarget];
    ResourceData& callerData = m_resourceData[_pSource];
    callerData.onChangedScopedConnections.emplace_back(targetData.onChangeSignal.ConnectScoped(_delegate));
}

void ResourceManager::NotifyResourceChange(
    const Ref<Resource>&      _pResource,
    const eResourceChangeType _type) const
{
    JAM_ASSERT(_pResource, "Cannot notify change for a null resource.");

    auto iter = m_resourceData.find(_pResource);
    if (iter != m_resourceData.end())
    {
        const ResourceData& resourceData = iter->second;
        resourceData.onChangeSignal.Emit(_pResource, _type);
    }
}

void ResourceManager::CreateInstance()
{
    JAM_ASSERT(s_pInstance == nullptr, "ResourceManager instance already exists.");
    s_pInstance = new ResourceManager();
}

void ResourceManager::DestroyInstance()
{
    JAM_ASSERT(s_pInstance != nullptr, "ResourceManager instance does not exist.");
    delete s_pInstance;
    s_pInstance = nullptr;
}

ResourceManager& ResourceManager::GetInstance()
{
    JAM_ASSERT(s_pInstance != nullptr, "ResourceManager instance does not exist.");
    return *s_pInstance;
}

void ResourceManager::CleanUpGarbage()
{
    for (auto it = m_shaderSources.begin(); it != m_shaderSources.end();)
    {
        const Ref<ShaderSource>& pSource = it->second;
        if (pSource->GetRefCount() == 1)   // only held by the map
        {
            it = m_shaderSources.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = m_resourceData.begin(); it != m_resourceData.end();)
    {
        const Ref<Resource>& pResource = it->first;
        if (pResource->GetRefCount() == 1)   // only held by the map
        {
            it = m_resourceData.erase(it);
        }
        else
        {
            ++it;
        }
    }
}