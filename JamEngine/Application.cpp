
#include "pch.h"

#include "Application.h"

#include "AssetManager.h"
#include "ComponentDatabase.h"
#include "Editor.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "ImGuiEngine.h"
#include "Input.h"
#include "SceneManager.h"
#include "Timer.h"
#include "WindowsPlatform.h"
#include "Events.h"
#include "SceneDatabase.h"

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

    TickTimer timer = {};
    timer.Start();
    while (m_bRunning)
    {
        if (!GetWindowsPlatform().PollEvents())
        {
            m_commandQueue.Execute();   // execute submitted commands

            timer.Tick();
            m_deltaTimeSec = timer.GetDeltaFloat(eSIPrefix::Sec);
            m_framerate    = 1.f/ m_deltaTimeSec;

            Update(m_deltaTimeSec);
            Render();

            GetImGuiEngine().BeginFrame();
            ImGuiRender();
            GetImGuiEngine().EndFrame();

            GetGraphics().Frame(m_bVsync);   // present
            GetInput().Update();             // update input
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

    OnEvent(_event);
    GetWindowsPlatform().OnEvent(_event);
    GetGraphics().OnEvent(_event);

    if (m_coreFlags & eAppCoreFlags::AssetManager)
    {
        GetAssetManager().OnEvent(_event);
    }

    if (m_coreFlags & eAppCoreFlags::Editor)
    {
        GetEditor().OnEvent(_event);
    }

    if (m_coreFlags & eAppCoreFlags::SceneManager)
    {
        GetSceneManager().OnEvent(_event);
    }

    GetInput().OnEvent(_event);
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

bool Application::IsCoreFlagsEnabled(
    const eAppCoreFlags _flags) const
{
    return m_coreFlags & _flags;
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
    const StringView                _appName,
    const Flags<eAppCoreFlags> _initFlags)
    : m_appName(_appName)
    , m_coreFlags(_initFlags)
    , m_workingDir(FileSystem::Current())
{
    s_pInstance = this;

    m_contentsDir = m_workingDir;
    m_contentsDir.Join("Contents/");
    m_contentsDir.Simplify();

    m_cacheDir = m_workingDir;
    m_cacheDir.Join("Caches/");
    m_cacheDir.Simplify();
}

void Application::Update(float _dt)
{
    if (m_coreFlags & eAppCoreFlags::Editor)
    {
        JAM_ASSERT(m_coreFlags & eAppCoreFlags::AssetManager, "AssetManager subsystem is required to update application when Editor subsystem is enabled.");
        GetEditor().Update(m_deltaTimeSec);
    }
    else
    {
        JAM_ASSERT(m_coreFlags & eAppCoreFlags::SceneManager, "SceneManager subsystem is required to update application when Editor subsystem is not enabled.");
        GetSceneManager().Update(m_deltaTimeSec);
    }
}

void Application::Render()
{
    if (m_coreFlags & eAppCoreFlags::Editor)
    {
        JAM_ASSERT(m_coreFlags & eAppCoreFlags::AssetManager, "AssetManager subsystem is required to update application when Editor subsystem is enabled.");
        GetEditor().Render();
    }
    else
    {
        JAM_ASSERT(m_coreFlags & eAppCoreFlags::SceneManager, "SceneManager subsystem is required to update application when Editor subsystem is not enabled.");
        GetSceneManager().RenderRuntime();
    }
}

void Application::ImGuiRender()
{
    if (m_coreFlags & eAppCoreFlags::Editor)
    {
        JAM_ASSERT(m_coreFlags & eAppCoreFlags::AssetManager, "AssetManager subsystem is required to update application when Editor subsystem is enabled.");
        GetEditor().ImGuiRender();
    }

    JAM_ASSERT(m_coreFlags & eAppCoreFlags::SceneManager, "SceneManager subsystem is required to update application when Editor subsystem is not enabled.");
    GetSceneManager().ImGuiRender();
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
    Log::CreateInstance();

    // init window
    WindowsPlatform::CreateInstance();
    GetWindowsPlatform().SetTitle(m_appName);

    // init graphics
    const auto [w, h] = GetWindowsPlatform().GetClientResolution();
    const HWND hWnd   = GetWindowsPlatform().GetPlatformHandle();
    Graphics::CreateInstance(hWnd, w, h, m_coreFlags.IsEnabled(eAppCoreFlags::AllowTearing));

    // init input
    Input::CreateInstance();

    // init asset manager
    if (m_coreFlags & eAppCoreFlags::AssetManager)
    {
        FileSystem::MakeDirectory(m_contentsDir);
        Expect<Scoped<FileLock>> pContentsDirLock = FileLock::Create(m_contentsDir, eFileLockMode::ReadWrite);
        if (!pContentsDirLock)
        {
            JAM_CRASH("Failed to lock contents directory '{}': {}", m_contentsDir, pContentsDirLock.GetError().ToErrorMessage());
        }
        m_pContentsDirLock = pContentsDirLock.Take();

        FileSystem::MakeDirectory(m_cacheDir);
        Expect<Scoped<FileLock>> pCacheDirLock = FileLock::Create(m_cacheDir, eFileLockMode::ReadWrite);
        if (!pCacheDirLock)
        {
            JAM_CRASH("Failed to lock cache directory '{}': {}", m_cacheDir, pCacheDirLock.GetError().ToErrorMessage());
        }
        m_pCacheDirLock = pCacheDirLock.Take();

        AssetManager::CreateInstance();
    }

    // init scene manager
    if (m_coreFlags & eAppCoreFlags::SceneManager)
    {
        SceneManager::CreateInstance();
    }

    // init imgui glue
    if (m_coreFlags & eAppCoreFlags::ImGuiEngine)
    {
        ImGuiEngine::CreateInstance(hWnd);
    }

    // init editor if need
    if (m_coreFlags & eAppCoreFlags::Editor)
    {
        JAM_ASSERT(m_coreFlags & eAppCoreFlags::AssetManager, "AssetManager subsystem is required to initialize editor system.");
        JAM_ASSERT(m_coreFlags & eAppCoreFlags::SceneManager, "SceneManager subsystem is required to initialize editor system.");
        JAM_ASSERT(m_coreFlags & eAppCoreFlags::ImGuiEngine, "Editor subsystem requires ImGuiEngine subsystem.");

        Editor::CreateInstance();
        m_coreFlags |= eAppCoreFlags::Editor;
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
    if (m_coreFlags & eAppCoreFlags::Editor)
    {
        Editor::DestroyInstance();
    }

    // shutdown imgui glue
    if (m_coreFlags & eAppCoreFlags::ImGuiEngine)
    {
        ImGuiEngine::DestroyInstance();
    }

    // shutdown scene manager
    if (m_coreFlags & eAppCoreFlags::SceneManager)
    {
        SceneManager::DestroyInstance();
    }

    // shutdown asset manager
    if (m_coreFlags & eAppCoreFlags::AssetManager)
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
    Log::DestroyInstance();
}

}   // namespace Jam
