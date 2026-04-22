#include "pch.h"
#include "ResourceManager.h"

void ResourceManager::OnUpdate(
    const float _dt)
{
    m_garbageCollectCounterSec += _dt;
    if (m_garbageCollectCounterSec >= kGarbageCollectInterval)
    {
        CleanUpGarbage();
        m_garbageCollectCounterSec = 0.f;
    }
}

void ResourceManager::OnEvent(Event& _event)
{
}

Expect<Ref<ShaderSource>> ResourceManager::LoadShaderSource(
    const StringView _path,
    const bool       _bReload)
{
    const String absPath = FileSystem::Absolute(_path);
    const auto   iter    = m_shaderSources.find(absPath);
    if (iter != m_shaderSources.end())
    {
        Ref<ShaderSource>& pSource = iter->second;

        if (_bReload)
        {
            if (const Error err = pSource->Load())
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

void ResourceManager::OnFileRenamedEvent_(
    const FileRenamedEvent& _event)
{
}

void ResourceManager::OnFileDeletedEvent_(
    const FileDeletedEvent& _event)
{
    auto it = m_shaderSources.find(_event.GetPath());
    if (it != m_shaderSources.end())
    {
        m_shaderSources.erase(it);
    }
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
}