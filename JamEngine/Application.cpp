#include "pch.h"

#include "Application.h"

#include "AssetManager.h"
#include "ComponentDB.h"
#include "Editor.h"
#include "GlobalBuffer.h"
#include "Graphics.h"
#include "ImGuiGlue.h"
#include "Input.h"
#include "Magic.h"
#include "SceneDB.h"
#include "SceneManager.h"
#include "ScriptDB.h"

namespace jam
{

Application* Application::s_instance = nullptr;

void Application::Create(const Int32 _argc, char** _argv)
{
    JAM_ASSERT(s_instance == nullptr, "Application instance already exists");

#ifdef JAM_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // locale
    std::locale::global(std::locale(""));

    // init
    Log::Init();

    // create application
    s_instance = CreateApplication(_argc, _argv);

    // project file directory
    fs::create_directory(s_instance->m_contentsDir);

    // init window
    s_instance->m_window.Init();
    s_instance->m_window.SetTitle(s_instance->m_applicationName);

    // init graphics
    const auto [width, height] = s_instance->m_window.GetClientResolution();
    const HWND hwnd            = s_instance->m_window.GetPlatformHandle();
    Graphics::Init(hwnd, width, height);

    // init input
    Input::Init();

    // init asset manager
    AssetManager::Init();

    // init scene manager
    SceneManager::Init();

    // init imgui glue
    ImGuiGlue::Init(hwnd);

    // init editor if need
    if (s_instance->IsEditorEnabled())
    {
        Editor::Init();
    }

    // init done
    s_instance->m_bEventDispatchable = true;
    s_instance->m_bRunning           = true;
    Log::Trace("application '{}' started.", s_instance->m_applicationName);

    // init user application
    s_instance->OnCreate();
}

void Application::Destroy()
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(s_instance->m_bRunning, "Application is still running, cannot destroy instance");

    // shutdown start
    Log::Trace("application '{}' shutdown.", s_instance->m_applicationName);

    // destroy user application
    s_instance->OnDestroy();

    // close event dispatch
    s_instance->m_bEventDispatchable = false;

    // destroy db
    SceneDB::DestroyInstance();
    ScriptDB::DestroyInstance();
    ComponentDB::DestroyInstance();

    // shutdown window
    s_instance->m_window.Shutdown();

    // shutdown editor
    if (s_instance->IsEditorEnabled())
    {
        Editor::Shutdown();
    }

    // shutdown imgui glue
    ImGuiGlue::Shutdown();

    // shutdown scene manager
    SceneManager::Shutdown();

    // shutdown asset manager
    AssetManager::Shutdown();

    // shutdown input
    Input::Shutdown();

    // shutdown graphics lib
    Graphics::Shutdown();

    // destroy application instance
    JAM_DELETE(s_instance);

    // shutdown
    Log::Shutdown();
}

Application& Application::GetInstance()
{
    JAM_ASSERT(s_instance, "Application instance is null");
    return *s_instance;
}

Application::Application(const ApplicationCreateInfo& _info)
    : m_applicationName(_info.applicationName)
    , m_bEditorEnabled(_info.bEnableEditor)
    , m_workingDir(_info.workingDirectory)
    , m_contentsDir(m_workingDir / k_contentsDirName.GetPtr())
{
    JAM_ASSERT(fs::exists(_info.workingDirectory), "Working directory does not exist: {}", _info.workingDirectory.native());
}

void Application::OnWindowCloseEvent_(
    [[maybe_unused]] const WindowCloseEvent& _event)
{
    Quit();
}

Int32 Application::Run()
{
    m_timer.Start();

    while (m_bRunning)
    {
        if (!m_window.PollEvents())
        {
            m_commandQueue.Execute();   // execute submitted commands

            m_timer.Tick();
            m_deltaTimeSec = static_cast<float>(m_timer.GetDelta(eSIPrefix::Sec));
            m_frameRate    = 1.f / m_deltaTimeSec;

            // update loop
            if (m_bEditorEnabled)
            {
                Editor::Update(m_deltaTimeSec);
            }
            else
            {
                SceneManager::Update(m_deltaTimeSec);
            }

            // rendering loop
            if (m_bEditorEnabled)
            {
                Editor::Render();
                ImGuiGlue::BeginFrame();
                Editor::ImGuiRender();
                SceneManager::ImGuiRender();
                ImGuiGlue::EndFrame();
            }
            else
            {
                // runtime
                SceneManager::RenderRuntime();
                ImGuiGlue::BeginFrame();
                SceneManager::ImGuiRender();
                ImGuiGlue::EndFrame();
            }

            Graphics::Frame(m_bVsync);

            // update input
            Input::Update();
        }
    }

    return 0;
}

void Application::Quit()
{
    m_bRunning = false;
}

void Application::DispatchEvent(Event& _event)
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(_event.IsHandled() == false, "Event '{}' is already handled.", _event.GetTypeName());

    if (m_bEventDispatchable == false)
    {
        return;
    }

    // application level event handling
    HandleEvent<WindowCloseEvent>(_event, this, &Application::OnWindowCloseEvent_);

    m_window.OnEvent(_event);
    Graphics::OnEvent(_event);
    AssetManager::OnEvent(_event);
    if (m_bEditorEnabled)
    {
        Editor::OnEvent(_event);
    }
    SceneManager::OnEvent(_event);
    Input::OnEvent(_event);
}

void Application::SubmitCommand(const std::function<void()>& _command)
{
    JAM_ASSERT(_command, "Command is null");
    m_commandQueue.Submit(_command);
}

void Application::SetVsync(const bool _bVsync)
{
    m_bVsync = _bVsync;
}

bool Application::IsVsyncEnabled() const
{
    return m_bVsync;
}

StringView Application::GetApplicationName() const
{
    return m_applicationName;
}

WindowsPlatform& Application::GetPlatform()
{
    return m_window;
}

const WindowsPlatform& Application::GetPlatform() const
{
    return m_window;
}

const fs::path& Application::GetWorkingDirectory() const
{
    return m_workingDir;
}

const fs::path& Application::GetContentsDirectory() const
{
    return m_contentsDir;
}

fs::path Application::JoinWorkingDirectory(const fs::path& _dir) const
{
    return m_workingDir / _dir;
}

fs::path Application::JoinContentsDirectory(const fs::path& _dir) const
{
    return m_contentsDir / _dir;
}

bool Application::IsEditorEnabled() const
{
    return m_bEditorEnabled;
}

float Application::GetDeltaTimeSec() const
{
    return m_deltaTimeSec;
}

float Application::GetFrameRate() const
{
    return m_frameRate;
}

}   // namespace jam
