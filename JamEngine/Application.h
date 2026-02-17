#pragma once
#include "CommandQueue.h"
#include "Timer.h"

namespace Jam
{

class WindowCloseEvent;
class Event;

class Application
{
public:
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    [[nodiscard]] static Application& GetInstance();

    Int32 Run(Int32 _argc, char** _argv);
    void  Quit();

    void DispatchEvent(Event& _event);
    void SubmitCommandToMainThread(const Delegate<void()>& _command);

    void               SetVsync(bool _bVsync);
    [[nodiscard]] bool IsVsyncEnabled() const;

    [[nodiscard]] StringView GetAppName() const;
    [[nodiscard]] float      GetDeltaTimeSec() const;
    [[nodiscard]] float      GetFrameRate() const;
    [[nodiscard]] bool       IsEditorEnabled() const;

    [[nodiscard]] const Path& GetWorkingDirectory() const;
    [[nodiscard]] const Path& GetContentsDirectory() const;

protected:
    explicit Application(StringView _appName, bool _bEnableEditor = false);
    virtual ~Application() = default;

private:
    // base
    void Init_(Int32 _argc, char** _argv);
    void Shutdown_();

    // derived
    virtual void Init(Int32 _argc, char** _argv);
    virtual void Shutdown();

    void OnEvent_(Event& _event);
    void OnWindowCloseEvent_(const WindowCloseEvent& _event);

    String       m_appName        = {};
    bool         m_bRunning       = false;
    bool         m_bInited        = false;
    bool         m_bVsync         = false;
    bool         m_bEditorEnabled = false;
    CommandQueue m_commandQueue   = {};

    // timer
    TickTimer m_timer        = {};
    float     m_deltaTimeSec = 0.f;
    float     m_framerate    = 0.f;

    // project
    Path m_workingDir  = fs::current_path();
    Path m_contentsDir = {};
    Path m_cacheDir    = {};

    // singletone
    inline static Application* s_pInstance = nullptr;
};

}   // namespace Jam

#define JAM_APP Jam::Application::GetInstance()

#define JAM_MAIN_IMPL(TApp, _appName, _bEnableEditor, _argc, _argv) \
    TApp app { _appName, _bEnableEditor };                          \
    return app.Run(_argc, _argv);

#define JAM_MAIN(TApp, _appName, _bEnableEditor)                    \
    int main(int _argc, char** _argv)                               \
    {                                                               \
        JAM_MAIN_IMPL(TApp, _appName, _bEnableEditor, _argc, _argv) \
    }