
#include "pch.h"

#include "Application.h"

#include "AssetManager.h"
#include "ComponentDatabase.h"
#include "Editor.h"
#include "Graphics.h"
#include "ImGuiGlue.h"
#include "Input.h"
#include "SceneDatabase.h"

#include "SceneManager.h"
#include "WindowsPlatform.h"

namespace Jam
{

Application& Application::GetInstance()
{
    return *s_pInstance;
}

Int32 Application::Run(
    const Int32 _argc,
    char**      _argv)
{
    Init_(_argc, _argv);

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
                JAM_IMGUI.BeginFrame();
                JAM_EDITOR.ImGuiRender();
                JAM_SCENE_MGR.ImGuiRender();
                JAM_IMGUI.EndFrame();
            }
            else
            {
                // runtime
                JAM_SCENE_MGR.RenderRuntime();
                JAM_IMGUI.BeginFrame();
                JAM_SCENE_MGR.ImGuiRender();
                JAM_IMGUI.EndFrame();
            }

            JAM_GFX.Frame(m_bVsync);

            // update input
            JAM_INPUT.Update();
        }
    }

    Shutdown_();
    return 0;
}

void Application::Quit()
{
    m_bRunning = false;
}

void Application::DispatchEvent(Event& _event)
{
    JAM_ASSERT(_event.IsHandled() == false, "Event '{}' is already handled.", _event.GetTypeName());

    // not initialized other systems
    if (m_bInited == false)
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

StringView Application::GetAppName() const
{
    return m_appName;
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

Application::Application(
    const StringView _appName,
    const bool       _bEnableEditor)
    : m_appName(_appName.ToString())
    , m_bEditorEnabled(_bEnableEditor)
    , m_workingDir(fs::current_path())
    , m_contentsDir(m_workingDir / "Contents")
    , m_cacheDir(m_workingDir / "Caches")
{
    s_pInstance = this;
}

void Application::Init_(
    const Int32 _argc,
    char**      _argv)
{
    JAM_ASSERT(!m_bInited, "Application is already initialized.");

    if (JAM_DEBUG_ENABLED)   // memory debugger
    {
#ifdef JAM_DEBUG_BREAK_ALLOC
        _CrtSetBreakAlloc(JAM_DEBUG_BREAK_ALLOC);
#endif
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
    JAM_WIN.SetTitle(m_appName);

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
    m_bInited  = true;
    m_bRunning = true;

    // init deriven type
    Init(_argc, _argv);

    // done
    JAM_LOG.Info("app '{}' initialization complete.", m_appName);
}

void Application::Init(
    const Int32 _argc,
    char**      _argv)
{
}

void Application::Shutdown_()
{
    JAM_ASSERT(m_bInited, "Application is not initialized.");

    // shutdown start
    JAM_LOG.Info("app '{}' is shutting down...", m_appName);
    m_bInited = false;
    Shutdown();   // shutdown derived type

    // destroy db
    ComponentDatabase::DestroyInstance();
    SceneDatabase::DestroyInstance();

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

    // shutdown
    JAM_LOG.Trace("app '{}' shutdown complete.", m_appName);
    Log::DestroyInstance();
}

void Application::Shutdown()
{
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
