#pragma once

#include "Event.h"
#include "Timer.h"
#include "Window.h"

namespace jam
{

struct CommandLineArguments
{
    NODISCARD std::string_view GetArgument(const int _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < argCount, "Argument index out of range");
        return args[_index];
    }

    char** args     = nullptr;
    int    argCount = 0;
};

struct ApplicationCreateInfo
{
    std::string          applicationName = "jam engine application";
    WindowInitializeInfo windowInfo      = {};   // window information

    // rendering
    bool bHDR   = true;
    bool bVSync = true;
};

// 1. implement CreateApplication, OnCreate, and OnDestroy in your application
// 2. call Application::CreateInstance in your main function
// 3. Application::CreateInstance -> Application::GetInstance().Run() -> Application::DestroyInstance()
// this is fixed jam application flow
// if you use built-in main function. You don't have to worry about these flows.
// recommend use JAM_ENTRY_POINT_MAIN or JAM_ENTRY_POINT_WIN_MAIN to define your main function

class Application
{
public:
    virtual ~Application() = default;

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    int  Run();   // do not call this function yourself
    void Quit();

    void DispatchEvent(Event& _event) const;

    // singletone accessor
    static void                   CreateInstance(const jam::CommandLineArguments& _args);
    static void                   DestroyInstance();
    static NODISCARD Application& GetInstance();

protected:
    explicit Application(const ApplicationCreateInfo& _info);

private:
    // application interface
    virtual void OnCreate()  = 0;   // implement this your application
    virtual void OnDestroy() = 0;   // implement this your application

    std::string m_applicationName = {};
    bool        m_bRunning        = false;

    Timer  m_timer  = {};   // application timer
    Window m_window = {};   // main window

    // singleton instance
    static Application* s_instance;
};

}   // namespace jam

// public API - do not call this
NODISCARD jam::Application* CreateApplication(const jam::CommandLineArguments& _args);   // you should define this function in your main file
NODISCARD jam::Application& GetApplication();