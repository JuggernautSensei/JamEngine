#pragma once
#include "CommandQueue.h"
#include "FileLock.h"

namespace Jam
{

class WindowClosedEvent;
class Event;

enum class eAppSubSystemFlags : UInt32
{
    None         = 0,
    SceneManager = 1 << 0,
    AssetManager = 1 << 1,
    ImGuiEngine  = 1 << 2,
    Editor       = 1 << 3,
};

constexpr Flags<eAppSubSystemFlags> k_runtimeDefaultSubSystems = {
    eAppSubSystemFlags::SceneManager,
    eAppSubSystemFlags::AssetManager,
    eAppSubSystemFlags::ImGuiEngine
};

constexpr Flags<eAppSubSystemFlags> k_editorDefaultSubSystems = {
    eAppSubSystemFlags::SceneManager,
    eAppSubSystemFlags::AssetManager,
    eAppSubSystemFlags::ImGuiEngine,
    eAppSubSystemFlags::Editor
};

class Application
{
public:
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    [[nodiscard]] static Application& GetInstance();

    Int32 Run(Int32 _argc, char** _argv);   // main loop. not recommended to override this method.
    void  Quit();

    void DispatchEvent(Event& _event);
    void SubmitCommandToMainThread(const Delegate<void()>& _command);

    void               SetVsync(bool _bVsync);
    [[nodiscard]] bool IsVsyncEnabled() const;

    [[nodiscard]] StringView GetAppName() const;
    [[nodiscard]] float      GetDeltaTimeSec() const;
    [[nodiscard]] float      GetFrameRate() const;
    [[nodiscard]] bool       IsSubSystemEnabled(eAppSubSystemFlags _subSystem) const;

    [[nodiscard]] StringView GetWorkingDirectory() const;
    [[nodiscard]] StringView GetContentsDirectory() const;

    [[nodiscard]] std::thread::id GetMainThreadId() const;

protected:
    explicit Application(StringView _appName, Flags<eAppSubSystemFlags> _subSystems = k_runtimeDefaultSubSystems);
    virtual ~Application() = default;

    // can be overridden by derived type
    // if you want to override the main loop, you can override this method and call Update(), Render() and ImGuiRender() in your own way.
    // like this:
    // void MyApp::Update(float _dt)
    // {
    //    // do something before update
    //    Application::Update(_dt);   // call base class update to update subsystems and dispatch events. if you don't want to update subsystems and dispatch events, you can skip this call.
    //    // do something after update
    // }

    virtual void Update(float _dt);
    virtual void Render();
    virtual void ImGuiRender();

    // can be overridden by derived type
    virtual void Init(Int32 _argc, char** _argv);
    virtual void Shutdown();
    virtual void OnEvent(Event& _event);

private:
    void OnWindowClose_(WindowClosedEvent& _event);
    void CoreInit();
    void CoreShutdown();

    String                    m_appName      = {};
    bool                      m_bRunning     = false;
    bool                      m_bInited      = false;
    bool                      m_bVsync       = false;
    Flags<eAppSubSystemFlags> m_subSystems   = {};
    CommandQueue              m_commandQueue = {};
    std::thread::id           m_mainThreadId = {};

    // timer
    float m_deltaTimeSec = 0.f;
    float m_framerate    = 0.f;

    // project
    String           m_workingDir       = {};
    String           m_contentsDir      = {};
    String           m_cacheDir         = {};
    Scoped<FileLock> m_pContentsDirLock = nullptr;
    Scoped<FileLock> m_pCacheDirLock    = nullptr;

    // singletone
    inline static Application* s_pInstance = nullptr;
};

[[nodiscard]] inline Application& GetApp()
{
    return Application::GetInstance();
}

}   // namespace Jam

#define JAM_MAIN_IMPL(TApp, _appName, _flags, _argc, _argv) \
    TApp app { _appName, _flags };                          \
    return app.Run(_argc, _argv);

#define JAM_MAIN(TApp, _appName, _flags)                    \
    int main(int _argc, char** _argv)                       \
    {                                                       \
        JAM_MAIN_IMPL(TApp, _appName, _flags, _argc, _argv) \
    }