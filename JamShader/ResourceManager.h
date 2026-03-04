#pragma once

class ResourceManager
{
    struct ResourceData
    {
        OnChangeSignal                   onChangeSignal             = {};   // my observers. emit when I change
        Vector<OnChangeScopedConnection> onChangedScopedConnections = {};   // my connections to other resources. automatically disconnect when I die
    };

public:
    ResourceManager(const ResourceManager&)            = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&)                 = delete;
    ResourceManager& operator=(ResourceManager&&)      = delete;

    // clear carbage
    void OnUpdate(float _dt);

    // shader resourc
    Expect<Ref<ShaderSource>> LoadShaderSource(StringView _path, bool _bReload = false);
    [[nodiscard]] bool        HasShaderSource(StringView _path) const;

    // resource change signal. disconnect is automatically handled by ResourceManager when either source or target is destroyed
    void ConnectResourceOnChange(const Ref<Resource>& _pSource, const OnChangeDelegate& _delegate, const Ref<Resource>& _pTarget);
    void NotifyResourceChange(const Ref<Resource>& _pResource, eResourceChangeType _type) const;

    // garbage collection
    void CleanUpGarbage();

    // singleton
    static void                           CreateInstance();
    static void                           DestroyInstance();
    [[nodiscard]] static ResourceManager& GetInstance();

private:
    ResourceManager()  = default;
    ~ResourceManager() = default;

    StringHashMap<Ref<ShaderSource>>     m_shaderSources = {};
    HashMap<Ref<Resource>, ResourceData> m_resourceData  = {};

    float                  m_garbageCollectCounterSec = 0.f;
    static constexpr float k_garbageCollectInterval   = 5.f;   // seconds

    inline static ResourceManager* s_pInstance = nullptr;
};

[[nodiscard]] inline ResourceManager& GetResourceManager()
{
    return ResourceManager::GetInstance();
}