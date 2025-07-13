#include "pch.h"

#include "Application.h"

namespace jam
{

Application* Application::s_instance = nullptr;

void Application::CreateInstance(const jam::CommandLineArguments& _args)
{
    JAM_ASSERT(s_instance == nullptr, "Application instance already exists");
    s_instance = CreateApplication(_args);

    // my on creae routine
    Log::Initialize();

    // create routine of child application
    s_instance->OnCreate();
}

void Application::DestroyInstance()
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(s_instance->m_bRunning == false, "Application is still running, cannot destroy instance");

    // destroy routine of child application
    s_instance->OnDestroy();

    // my shutdown routine
    Log::Shutdown();

    // destroy
    delete s_instance;
    s_instance = nullptr;
}

Application& Application::GetInstance()
{
    JAM_ASSERT(s_instance, "Application instance is null");
    return *s_instance;
}

Application::Application(const ApplicationCreateInfo& _info)
    : m_applicationName(_info.applicationName)
{
    s_instance = this;
}

int Application::Run()
{
    m_timer.Reset();
    m_timer.Start();

    while (m_bRunning)
    {
        m_timer.Tick();
        float deltaTime = m_timer.GetDurationSec();

        if (!m_window.PollEvents())
        {
            // do something
        }
    }

    return 0;
}

void Application::Quit()
{
    JAM_ASSERT(m_bRunning, "Application is not running");
    m_bRunning = false;
}

void Application::DispatchEvent(Event& _event) const
{
    JAM_ASSERT(_event.IsHandled() == false, "Event '{}' is already handled.", _event.GetName());

    m_window.OnEvent(_event);
}

}   // namespace jam

jam::Application& GetApplication()
{
    return jam::Application::GetInstance();
}