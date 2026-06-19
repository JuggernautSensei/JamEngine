
#include "pch.h"

#include "Application.h"

#include "AssetManager.h"
#include "ComponentDatabase.h"
#include "Graphics.h"
#include "ImGuiEngine.h"
#include "Input.h"
#include "SceneManager.h"
#include "Timer.h"
#include "WindowsPlatform.h"
#include "Events.h"
#include "FileSystem.h"
#include "SceneDatabase.h"
// #include "Editor.h"

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
    CoreInit();
    Init(_argc, _argv);

    m_timer.Start();

    WindowsPlatform& win   = GetWindowsPlatform();
    ImGuiEngine&     imgui = GetImGuiEngine();
    Graphics&        gfx   = GetGraphics();
    Input&           input = GetInput();

    while (m_bRunning)
    {
        if (!win.PollEvents())
        {
            m_commandQueue.Execute();   // execute submitted commands

            m_timer.Tick();
            const float dtSec = m_timer.GetDeltaFloat(eSIPrefix::Sec);
            Update(dtSec);
            Render();

            imgui.BeginFrame();
            ImGuiRender();
            imgui.EndFrame();

            gfx.Frame();      // present
            input.Update();   // update input
        }
    }

    m_bInited = false;   // prevent dispatching events during shutdown
    Shutdown();
    CoreShutdown();
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

    WindowsPlatform& win   = GetWindowsPlatform();
    Graphics&        gfx   = GetGraphics();
    Input&           input = GetInput();

    OnEvent(_event);
    win.OnEvent(_event);
    gfx.OnEvent(_event);

    if (m_flags & eAppInitFlags::Subsystem_AssetManager)
    {
        AssetManager& am = GetAssetManager();
        am.OnEvent(_event);
    }

    if (m_flags & eAppInitFlags::Subsystem_Editor)
    {
        // Editor& editor = GetEditor();
        // editor.OnEvent(_event);
    }

    if (m_flags & eAppInitFlags::Subsystem_SceneManager)
    {
        SceneManager& sm = GetSceneManager();
        sm.OnEvent(_event);
    }

    input.OnEvent(_event);
}

void Application::SubmitCommandToMainThread(
    const Delegate<void()>& _command)
{
    JAM_ASSERT(_command, "Command is null");
    m_commandQueue.Submit(_command);
}

StringView Application::GetAppName() const
{
    return m_appName;
}

Flags<eAppInitFlags> Application::GetInitFlags() const
{
    return m_flags;
}

const TickTimer& Application::GetTimer() const
{
    return m_timer;
}

StringView Application::GetWorkingDirectory() const
{
    return m_workingDir;
}

StringView Application::GetContentsDirectory() const
{
    return m_contentsDir;
}

std::thread::id Application::GetMainThreadId() const
{
    return m_mainThreadId;
}

Application::Application(
    const StringView           _appName,
    const Flags<eAppInitFlags> _flags)
    : m_appName(_appName)
    , m_flags(_flags)
    , m_workingDir(FS::Current())
{
    s_pInstance = this;

    m_contentsDir = m_workingDir;
    m_contentsDir.Join("Contents/");
    m_contentsDir.Simplify();

    m_cacheDir = m_workingDir;
    m_cacheDir.Join("Caches/");
    m_cacheDir.Simplify();
}

void Application::Update(const float _dt)
{
    if (m_flags & eAppInitFlags::Subsystem_Editor)
    {
        JAM_ASSERT(m_flags & eAppInitFlags::Subsystem_AssetManager, "AssetManager subsystem is required to update application when Editor subsystem is enabled.");
        //  Editor& editor = GetEditor();
        //  editor.Update(_dt);
    }
    else
    {
        JAM_ASSERT(m_flags & eAppInitFlags::Subsystem_SceneManager, "SceneManager subsystem is required to update application when Editor subsystem is not enabled.");
        SceneManager& sm = GetSceneManager();
        sm.Update(_dt);
    }
}

void Application::Render()
{
    if (m_flags & eAppInitFlags::Subsystem_Editor)
    {
        JAM_ASSERT(m_flags & eAppInitFlags::Subsystem_AssetManager, "AssetManager subsystem is required to update application when Editor subsystem is enabled.");
        // Editor& editor = GetEditor();
        // editor.Render();
    }
    else
    {
        JAM_ASSERT(m_flags & eAppInitFlags::Subsystem_SceneManager, "SceneManager subsystem is required to update application when Editor subsystem is not enabled.");
        SceneManager& sm = GetSceneManager();
        sm.RenderRuntime();
    }
}

void Application::ImGuiRender()
{
    if (m_flags & eAppInitFlags::Subsystem_Editor)
    {
        JAM_ASSERT(m_flags & eAppInitFlags::Subsystem_AssetManager, "AssetManager subsystem is required to update application when Editor subsystem is enabled.");
        // Editor& editor = GetEditor();
        // editor.ImGuiRender();
    }

    JAM_ASSERT(m_flags & eAppInitFlags::Subsystem_SceneManager, "SceneManager subsystem is required to update application when Editor subsystem is not enabled.");
    SceneManager& sm = GetSceneManager();
    sm.ImGuiRender();
}

void Application::Init(
    const Int32 _argc,
    char**      _argv)
{
}

void Application::Shutdown()
{
}

void Application::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<WindowClosedEvent>(this, &Application::OnWindowClose_);
}

void Application::OnWindowClose_(
    WindowClosedEvent&)
{
    Quit();
}

void Application::CoreInit()
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
    LogSystem::CreateInstance();

    // init window
    WindowsPlatform::CreateInstance();
    WindowsPlatform& win = GetWindowsPlatform();
    win.SetTitle(m_appName);

    // init graphics
    {
        const Vec2i res = win.GetClientResolution();

        GraphicsInitInfo info = {};
        info.platformHandle   = win.GetPlatformHandle();
        info.bConnectDebugger = m_flags.IsEnabled(eAppInitFlags::Graphics_ConnectDebugger);
        info.backBufferWidth  = res.x;
        info.backBufferHeight = res.y;
        info.numBackBuffer    = m_flags & eAppInitFlags::Graphics_QuadrupleBuffering ? 4
                              : m_flags & eAppInitFlags::Graphics_TripleBuffering    ? 3
                                                                                     : 2;

        Flags<eAppInitFlags> hdrFlags = m_flags.WithFilter(eAppInitFlags::Graphics_BackBufferHDR10, eAppInitFlags::Graphics_BackBufferHDR16);
        info.backBufferFormat         = hdrFlags == eAppInitFlags::Graphics_BackBufferHDR10 ? eTextureFormat::RGB10A2_UNorm
                                      : hdrFlags == eAppInitFlags::Graphics_BackBufferHDR16 ? eTextureFormat::RGBA16_Float
                                                                                            : eTextureFormat::RGBA8_UNorm;

        Flags<eAppInitFlags> msaaFlags = m_flags.WithFilter(eAppInitFlags::Graphics_BackBufferMSAAx4, eAppInitFlags::Graphics_BackBufferMSAAx16);
        info.backBufferSamples         = msaaFlags == eAppInitFlags::Graphics_BackBufferMSAAx4  ? 4
                                       : msaaFlags == eAppInitFlags::Graphics_BackBufferMSAAx8  ? 8
                                       : msaaFlags == eAppInitFlags::Graphics_BackBufferMSAAx16 ? 16
                                                                                                : 1;

        Graphics::CreateInstance(info);
    }

    // init input
    Input::CreateInstance();

    // init asset manager
    if (m_flags & eAppInitFlags::Subsystem_AssetManager)
    {
        AssetManager::CreateInstance();
    }

    // init scene manager
    if (m_flags & eAppInitFlags::Subsystem_SceneManager)
    {
        SceneManager::CreateInstance();
    }

    // init imgui glue
    if (m_flags & eAppInitFlags::Subsystem_ImGuiEngine)
    {
        ImGuiEngine::CreateInstance(win.GetPlatformHandle());
    }

    // init editor if need
    if (m_flags & eAppInitFlags::Subsystem_Editor)
    {
        JAM_ASSERT(m_flags & eAppInitFlags::Subsystem_AssetManager, "AssetManager subsystem is required to initialize editor system.");
        JAM_ASSERT(m_flags & eAppInitFlags::Subsystem_SceneManager, "SceneManager subsystem is required to initialize editor system.");
        JAM_ASSERT(m_flags & eAppInitFlags::Subsystem_ImGuiEngine, "Editor subsystem requires ImGuiEngine subsystem.");

        //     Editor::CreateInstance();
        m_flags |= eAppInitFlags::Subsystem_Editor;
    }

    // init done
    m_bInited      = true;
    m_bRunning     = true;
    m_mainThreadId = std::this_thread::get_id();
}

void Application::CoreShutdown()
{
    JAM_ASSERT(!m_bInited, "Application is not initialized.");
    JAM_ASSERT(!m_bRunning, "Application is still running.");

    // destroy db
    ComponentDatabase::DestroyInstance();
    SceneDatabase::DestroyInstance();

    // shutdown editor
    if (m_flags & eAppInitFlags::Subsystem_Editor)
    {
        //  Editor::DestroyInstance();
    }

    // shutdown imgui glue
    if (m_flags & eAppInitFlags::Subsystem_ImGuiEngine)
    {
        ImGuiEngine::DestroyInstance();
    }

    // shutdown scene manager
    if (m_flags & eAppInitFlags::Subsystem_SceneManager)
    {
        SceneManager::DestroyInstance();
    }

    // shutdown asset manager
    if (m_flags & eAppInitFlags::Subsystem_AssetManager)
    {
        AssetManager::DestroyInstance();
    }

    // shutdown input
    Input::DestroyInstance();

    // shutdown graphics lib
    Graphics::DestroyInstance();

    // shutdown window
    WindowsPlatform::DestroyInstance();

    // shutdown
    JAM_LOG_TRACE("app '{}' shutdown complete.", m_appName);
    LogSystem::DestroyInstance();
}

}   // namespace Jam
