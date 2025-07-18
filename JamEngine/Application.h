#pragma once

#include "CommandQueue.h"
#include "Event.h"
#include "Timer.h"
#include "Window.h"
#include "ILayer.h"

namespace jam
{

struct CommandLineArguments
{
    NODISCARD std::string_view GetArgument(const int _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < argCount, "Argument index out of range");
        return args[_index];
    }

    int    argCount = 0;
    char** args     = nullptr;
};

struct ApplicationCreateInfo
{
    std::string applicationName  = "jam engine application";
    fs::path    workingDirectory = fs::current_path();
    float       targetFrameRate  = 240.f;   // 0 -> unlimited frame rate
};

// 1. implement CreateApplication, OnCreate, and OnDestroy in your application
// 2. call Application::Create in your main function
// 3. Application::Create -> Application::GetPlatformInstance().Run() -> Application::Destroy()
// this is fixed jam application flow
// if you use built-in main function. You don't have to worry about these flows.
// recommend use JAM_MAIN or JAM_WIN_MAIN to define your main function

class Application
{
public:
    virtual ~Application() = default;

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    // core interface
    int  Run();   // do not call this function yourself
    void Quit();
    void DispatchEvent(Event& _event);
    void SubmitCommand(const std::function<void()>& _command);

    // layer interface
    void              PushBackLayer(std::unique_ptr<ILayer>&& _layer);
    void              PushFrontLayer(std::unique_ptr<ILayer>&& _pLayer);
    void              RemoveLayer(ILayer* _pLayer);
    NODISCARD ILayer* GetLayer(UInt32 _layerHash) const;

    // set properties
    void SetTargetFrameRate(float _fps);   // if _fps == 0 -> unlimited frame rate

    // getter
    NODISCARD Window& GetWindow();
    NODISCARD float   GetDeltaSecond() const;

    // singletone accessor
    static void                   Create(const jam::CommandLineArguments& _args);   // public constructor - call at the beginning of your main function
    static void                   Destroy();                                        // public destructor - call at the end of your main function
    static NODISCARD Application& GetInstance();

protected:
    explicit Application(const ApplicationCreateInfo& _info);

private:
    // application interface
    virtual void OnCreate()  = 0;   // implement this your application
    virtual void OnDestroy() = 0;   // implement this your application

    std::string m_applicationName  = {};
    fs::path    m_workingDirectory = {};
    bool        m_bRunning         = false;

    Window       m_window       = {};
    CommandQueue m_commandQueue = {};

    // timer
    TickTimer m_timer = {};

    // layer
    std::vector<std::unique_ptr<ILayer>> m_layers = {};   // layers stack

    // singleton instance
    static Application* s_instance;
};

}   // namespace jam

// public API
NODISCARD jam::Application* CreateApplication(const jam::CommandLineArguments& _args);   // you should define this function in your main file
NODISCARD jam::Application& GetApplication();