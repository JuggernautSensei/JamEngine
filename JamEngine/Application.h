#pragma once

#include "CommandQueue.h"
#include "Timer.h"
#include "WindowsPlatform.h"

namespace jam
{

struct ApplicationCreateInfo
{
    std::string applicationName  = "jam engine application";
    fs::path    workingDirectory = fs::current_path();
    bool        bEnableEditor    = false;
};

class Application
{
public:
    virtual ~Application() = default;

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    Int32 Run();
    void  Quit();

    void DispatchEvent(Event& _event);
    void SubmitCommand(const std::function<void()>& _command);

    void               SetVsync(bool _bVsync);
    [[nodiscard]] bool IsVsyncEnabled() const;

    [[nodiscard]] StringView             GetApplicationName() const;
    [[nodiscard]] WindowsPlatform&       GetPlatform();
    [[nodiscard]] const WindowsPlatform& GetPlatform() const;
    [[nodiscard]] float                  GetDeltaTimeSec() const;
    [[nodiscard]] float                  GetFrameRate() const;
    [[nodiscard]] bool                   IsEditorEnabled() const;

    [[nodiscard]] const fs::path& GetWorkingDirectory() const;
    [[nodiscard]] const fs::path& GetContentsDirectory() const;
    [[nodiscard]] fs::path        JoinWorkingDirectory(const fs::path& _dir) const;
    [[nodiscard]] fs::path        JoinContentsDirectory(const fs::path& _dir) const;

    static void Create(Int32 _argc, char** _argv);
    static void Destroy();

    static [[nodiscard]] Application& GetInstance();

protected:
    explicit Application(const ApplicationCreateInfo& _info);

private:
    void OnWindowCloseEvent_(const WindowCloseEvent& _event);

    virtual void OnCreate() {}
    virtual void OnDestroy() {}

    std::string                   m_applicationName    = {};
    bool                          m_bRunning           = false;
    bool                          m_bEventDispatchable = false;
    bool                          m_bVsync             = false;
    bool                          m_bEditorEnabled     = false;
    WindowsPlatform               m_window             = {};
    CommandQueue                  m_commandQueue       = {};

    // timer
    TickTimer m_timer        = {};
    float     m_deltaTimeSec = 0.f;
    float     m_frameRate    = 0.f;

    // project
    fs::path m_workingDir  = fs::current_path();
    fs::path m_contentsDir = {};

    // singletone
    static Application* s_instance;
};

}   // namespace jam

// this function must define in user's code
[[nodiscard]] jam::Application* CreateApplication(jam::Int32 argc, char** argv);
