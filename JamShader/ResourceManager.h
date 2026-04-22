#pragma once

class ResourceManager
{
public:
    ResourceManager(const ResourceManager&)            = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&)                 = delete;
    ResourceManager& operator=(ResourceManager&&)      = delete;

    // for clear carbage
    void OnUpdate(float _dt);
    void OnEvent(Event& _event);

    // resource cache
    Expect<Ref<ShaderSource>> LoadShaderSource(StringView _path, bool _bReload = false);
    [[nodiscard]] bool        HasShaderSource(StringView _path) const;

    // garbage collection
    void CleanUpGarbage();

    // singleton
    static void                           CreateInstance();
    static void                           DestroyInstance();
    [[nodiscard]] static ResourceManager& GetInstance();

private:
    ResourceManager()  = default;
    ~ResourceManager() = default;

    void OnFileRenamedEvent_(const FileRenamedEvent& _event);
    void OnFileDeletedEvent_(const FileDeletedEvent& _event);

    StringHashMap<Ref<ShaderSource>> m_shaderSources = {};

    float                  m_garbageCollectCounterSec = 0.f;
    static constexpr float kGarbageCollectInterval   = 5.f;   // seconds

    inline static ResourceManager* s_pInstance = nullptr;
};

[[nodiscard]] inline ResourceManager& GetResourceManager()
{
    return ResourceManager::GetInstance();
}