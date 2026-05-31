#pragma once
#include "CommandQueue.h"
#include "FileLock.h"
#include "Timer.h"

namespace Jam
{

class WindowClosedEvent;
class Event;

enum class eAppInitFlags : UInt32
{
    None = 0,

    // subsystem
    Subsystem_SceneManager = 1 << 0,
    Subsystem_AssetManager = 1 << 1,
    Subsystem_ImGuiEngine  = 1 << 2,
    Subsystem_Editor       = 1 << 3,

    // graphics
    Graphics_AllowTearing = 1 << 4,

    Graphics_BackBufferHDR10 = 1 << 5,
    Graphics_BackBufferHDR16 = 2 << 5,

    Graphics_BackBufferMSAAx4  = 1 << 7,
    Graphics_BackBufferMSAAx8  = 2 << 7,
    Graphics_BackBufferMSAAx16 = 3 << 7,

    Graphics_GraphicsDebugger = 1 << 9,
};

constexpr Flags<eAppInitFlags> kDefaultAppInitFlags = eAppInitFlags::Subsystem_SceneManager | eAppInitFlags::Subsystem_AssetManager | eAppInitFlags::Subsystem_ImGuiEngine;
constexpr Flags<eAppInitFlags> kEditorAppInitFlags  = kDefaultAppInitFlags | eAppInitFlags::Subsystem_Editor;

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

    [[nodiscard]] StringView       GetAppName() const;
    [[nodiscard]] const TickTimer& GetTimer() const;
    [[nodiscard]] bool             IsAppInitFlagsEnabled(eAppInitFlags _flags) const;

    [[nodiscard]] StringView GetWorkingDirectory() const;
    [[nodiscard]] StringView GetContentsDirectory() const;

    [[nodiscard]] std::thread::id GetMainThreadId() const;

protected:
    explicit Application(StringView _appName, Flags<eAppInitFlags> _flags);
    virtual ~Application() = default;

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

    String               m_appName      = {};
    bool                 m_bRunning     = false;
    bool                 m_bInited      = false;
    bool                 m_bVsync       = false;
    CommandQueue         m_commandQueue = {};
    std::thread::id      m_mainThreadId = {};
    Flags<eAppInitFlags> m_flags        = {};

    // timer
    TickTimer m_timer = {};

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
