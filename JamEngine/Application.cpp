
#include "pch.h"

#include "Application.h"

#include "AssetManager.h"
#include "ReflectionDB.h"
#include "Editor.h"
#include "Graphics.h"
#include "ImGuiGlue.h"
#include "Input.h"

#include "SceneManager.h"
#include "WindowsPlatform.h"

namespace Jam
{

void Application::CreateInstance(
    const Int32 _argc,
    char**      _argv)
{
    JAM_ASSERT(s_pInstance == nullptr, "Application instance already exists");
    s_pInstance = CreateApplication(_argc, _argv);
}

void Application::DestroyInstance()
{
    JAM_ASSERT(s_pInstance, "Application instance is null");
    delete s_pInstance;
}

Application& Application::GetInstance()
{
    return *s_pInstance;
}

Int32 Application::Run()
{
    m_timer.Start();

    while (m_bRunning)
    {
        if (!JAM_WIN.PollEvents())
        {
            m_commandQueue.Execute();   // execute submitted commands

            m_timer.Tick();
            m_deltaTimeSec = m_timer.GetDeltaFloat(eSIPrefix::Sec);
            m_framerate    = Div(1.f, m_deltaTimeSec);

            // update loop
            if (m_bEditorEnabled)
            {
                JAM_EDITOR.Update(m_deltaTimeSec);
            }
            else
            {
                JAM_SCENE_MGR.Update(m_deltaTimeSec);
            }

            // rendering loop
            if (m_bEditorEnabled)
            {
                JAM_EDITOR.Render();
                JAM_IMGUI_GLUE.BeginFrame();
                JAM_EDITOR.ImGuiRender();
                JAM_SCENE_MGR.ImGuiRender();
                JAM_IMGUI_GLUE.EndFrame();
            }
            else
            {
                // runtime
                JAM_SCENE_MGR.RenderRuntime();
                JAM_IMGUI_GLUE.BeginFrame();
                JAM_SCENE_MGR.ImGuiRender();
                JAM_IMGUI_GLUE.EndFrame();
            }

            JAM_GFX.Frame(m_bVsync);

            // update input
            JAM_INPUT.Update();
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
    JAM_ASSERT(_event.IsHandled() == false, "Event '{}' is already handled.", _event.GetTypeName());

    if (m_bEventDispatchable == false)
    {
        return;
    }

    OnEvent_(_event);
    JAM_WIN.OnEvent(_event);
    JAM_GFX.OnEvent(_event);
    JAM_ASSET_MGR.OnEvent(_event);

    if (m_bEditorEnabled)
    {
        JAM_EDITOR.OnEvent(_event);
    }

    JAM_SCENE_MGR.OnEvent(_event);
    JAM_INPUT.OnEvent(_event);
}

void Application::SubmitCommandToMainThread(
    const Delegate<void()>& _command)
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

float Application::GetDeltaTimeSec() const
{
    return m_deltaTimeSec;
}

float Application::GetFrameRate() const
{
    return m_framerate;
}

bool Application::IsEditorEnabled() const
{
    return m_bEditorEnabled;
}

const Path& Application::GetWorkingDirectory() const
{
    return m_workingDir;
}

const Path& Application::GetContentsDirectory() const
{
    return m_contentsDir;
}

Application::Application(const ApplicationCreateInfo& _info)
    : m_applicationName(_info.applicationName)
      , m_bEditorEnabled(_info.bEnableEditor)
      , m_workingDir(_info.workingDirectory)
      , m_contentsDir(m_workingDir / "Contents")
      , m_cacheDir(m_workingDir / "Caches")
{
    // 메모리 디버거
    if (JAM_DEBUG_ENABLED)
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }

    // locale
    std::locale::global(std::locale(""));

    // init
    Log::CreateInstance();

    // project file directory
    fs::create_directories(m_contentsDir);
    fs::create_directories(m_cacheDir);

    // init window
    WindowsPlatform::CreateInstance();
    JAM_WIN.SetTitle(m_applicationName);

    // init graphics
    const auto [w, h] = JAM_WIN.GetClientResolution();
    const HWND hWnd   = JAM_WIN.GetPlatformHandle();
    Graphics::CreateInstance(hWnd, w, h);

    // init input
    Input::CreateInstance();

    // init asset manager
    AssetManager::CreateInstance();

    // init scene manager
    SceneManager::CreateInstance();

    // init imgui glue
    ImGuiGlue::CreateInstance(hWnd);

    // init editor if need
    if (IsEditorEnabled())
    {
        Editor::CreateInstance();
    }

    // init done
    m_bEventDispatchable = true;
    m_bRunning           = true;
    JAM_LOG.Trace("application '{}' started.", m_applicationName);
}

Application::~Application()
{
    JAM_ASSERT(m_bRunning, "Application is still running, cannot destroy instance");

    // shutdown start
    JAM_LOG.Trace("application '{}' shutdown.", m_applicationName);

    // close event dispatch
    m_bEventDispatchable = false;

    // destroy db (this must destroy manually, cannot use onDestroyCallbacks)
    ReflectionDB::DestroyInstance();

    // shutdown window
    WindowsPlatform::DestroyInstance();

    // shutdown editor
    if (IsEditorEnabled())
    {
        Editor::DestroyInstance();
    }

    // shutdown imgui glue
    ImGuiGlue::DestroyInstance();

    // shutdown scene manager
    SceneManager::DestroyInstance();

    // shutdown asset manager
    AssetManager::DestroyInstance();

    // shutdown input
    Input::DestroyInstance();

    // shutdown graphics lib
    Graphics::DestroyInstance();

    JAM_LOG.Trace("application '{}' shutdown complete.", m_applicationName);
    JAM_LOG.Flush();

    // shutdown
    Log::DestroyInstance();
}

void Application::OnEvent_(
    Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<WindowCloseEvent>(this, &Application::OnWindowCloseEvent_);
}

void Application::OnWindowCloseEvent_(
    [[maybe_unused]] const WindowCloseEvent& _event)
{
    Quit();
}

}   // namespace Jam
