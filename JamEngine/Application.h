#pragma once
#include "CommandQueue.h"
#include "Timer.h"

namespace Jam
{

class WindowCloseEvent;
class Event;

struct ApplicationCreateInfo
{
    String applicationName  = "Jam engine application";
    Path   workingDirectory = fs::current_path();
    bool   bEnableEditor    = false;
};

class Application
{
public:
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    static void                       CreateInstance(Int32 _argc, char** _argv);
    static void                       DestroyInstance();
    [[nodiscard]] static Application& GetInstance();

    Int32 Run();
    void  Quit();

    void DispatchEvent(Event& _event);
    void SubmitCommandToMainThread(const Delegate<void()>& _command);

    void               SetVsync(bool _bVsync);
    [[nodiscard]] bool IsVsyncEnabled() const;

    [[nodiscard]] StringView GetApplicationName() const;
    [[nodiscard]] float      GetDeltaTimeSec() const;
    [[nodiscard]] float      GetFrameRate() const;
    [[nodiscard]] bool       IsEditorEnabled() const;

    [[nodiscard]] const Path& GetWorkingDirectory() const;
    [[nodiscard]] const Path& GetContentsDirectory() const;

protected:
    explicit Application(const ApplicationCreateInfo& _info);
    virtual ~Application();

private:
    void OnEvent_(Event& _event);
    void OnWindowCloseEvent_(const WindowCloseEvent& _event);

    String       m_applicationName    = {};
    bool         m_bRunning           = false;
    bool         m_bEventDispatchable = false;
    bool         m_bVsync             = false;
    bool         m_bEditorEnabled     = false;
    CommandQueue m_commandQueue       = {};

    // timer
    TickTimer m_timer        = {};
    float     m_deltaTimeSec = 0.f;
    float     m_framerate    = 0.f;

    // project
    Path m_workingDir  = fs::current_path();
    Path m_contentsDir = {};
    Path m_cacheDir     = {};

    // singletone
    inline static Application* s_pInstance = nullptr;
};

}   // namespace Jam

// this function must define in user's code
[[nodiscard]] Jam::Application* CreateApplication(Jam::Int32 argc, char** argv);

#define JAM_APP Jam::Application::GetInstance()