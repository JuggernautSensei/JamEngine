#include "pch.h"

#include "Application.h"

#include "AssetManager.h"
#include "Editor.h"
#include "Graphics.h"
#include "ImGuiGlue.h"
#include "Input.h"
#include "Magic.h"
#include "SceneDB.h"
#include "SceneManager.h"
#include "ScriptDB.h"

namespace jam
{

static void OnNewFailed()
{
    JAM_CRASH("Failed to create application instance. Please check your CreateApplication implementation.");
}

Application* Application::s_instance = nullptr;

void Application::Create(const CommandLineArguments& _args)
{
    JAM_ASSERT(s_instance == nullptr, "Application instance already exists");

#ifdef JAM_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // 로그 초기화
    Log::Init();

    // 싱글톤 객체 생성
    s_instance = CreateApplication(_args);

    // 어플리케이션 초기화 루틴
    {
        // 기본 속성 설정
        std::set_new_handler(OnNewFailed);
        std::locale::global(std::locale(""));

        // 파일 시스템 생성
        fs::create_directory(s_instance->m_contentsDir);

        // 윈도우 초기화
        s_instance->m_window.Init();
        s_instance->m_window.SetTitle(s_instance->m_applicationName);
        const auto [width, height] = s_instance->m_window.GetClientResolution();
        const HWND hwnd            = s_instance->m_window.GetPlatformHandle();

        // 렌더러 초기화
        Graphics::Init(hwnd, width, height);

        // 입력 초기화
        Input::Init();

        // asset manager 초기화
        AssetManager::Init();

        // 씬 매니저 초기화
        SceneManager::Init();

        // imgui 초기화
        ImGuiGlue::Init(hwnd);

        // editor 초기화
        if (s_instance->IsEditorEnabled())
        {
            Editor::Init();
        }
    }

    // 초기화 완료
    s_instance->m_bEventDispatchable = true;
    s_instance->m_bRunning           = true;
    Log::Trace("Application '{}' started.", s_instance->m_applicationName);

    // 사용자 정의 어플리케이션 초기화 루틴
    s_instance->OnCreate();
}

void Application::Destroy()
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(s_instance->m_bRunning == false, "Application is still running, cannot destroy instance");

    // 종료 시작
    Log::Trace("application '{}' shutdown.", s_instance->m_applicationName);

    // 사용자 정의 어플리케이션 종료 루틴
    s_instance->OnDestroy();

    // 이벤트 디스패치 비활성화
    s_instance->m_bEventDispatchable = false;

    // db 정리
    SceneDB::DestroyInstance();
    ScriptDB::DestroyInstance();
    ComponentDB::DestroyInstance();

    // 윈도우 종료
    s_instance->m_window.Shutdown();

    // 에디터 종료
    if (s_instance->IsEditorEnabled())
    {
        Editor::Shutdown();
    }

    // imgui 종료
    ImGuiGlue::Shutdown();

    // 씬 매니저 종료
    SceneManager::Shutdown();

    // 에셋 매니저 정리
    AssetManager::Shutdown();

    // 입력 종료
    Input::Shutdown();

    // 렌더러 종료
    Graphics::Shutdown();

    // 싱글톤 객체 삭제
    JAM_DELETE(s_instance);

    // 로그 종료
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
    , m_contentsDir(m_workingDir / k_contentsDirName)
{
    JAM_ASSERT(fs::exists(_info.workingDirectory), "Working directory does not exist: {}", _info.workingDirectory.native());
}

void Application::OnWindowCloseEvent_(const WindowCloseEvent& _event)
{
    Quit();
}

int Application::Run()
{
    m_timer.Start();

    while (m_bRunning)
    {
        if (!m_window.PollEvents())
        {
            m_commandQueue.Execute();   // 이전 프레임에서 수집한 커맨드 실행

            m_timer.Tick();
            m_deltaTimeSec = static_cast<float>(m_timer.GetDelta(eSIPrefix::Sec));
            m_frameRate    = 1.f / m_deltaTimeSec;

            // 업데이트
            if (m_bEditorEnabled)
            {
                Editor::Update(m_deltaTimeSec);
            }
            else
            {
                SceneManager::Update(m_deltaTimeSec);
            }

            // 렌더링
            {
                if (m_bEditorEnabled)
                {
                    Editor::Render();
                    ImGuiGlue::BeginFrame();
                    Editor::ImGuiRender();
                    SceneManager::ImGuiRender();
                    ImGuiGlue::EndFrame();
                }
                else   // 런타임
                {
                    SceneManager::RenderRuntime();
                    ImGuiGlue::BeginFrame();
                    SceneManager::ImGuiRender();
                    ImGuiGlue::EndFrame();
                }

                Graphics::Frame(m_bVsync);
            }

            // 인풋 갱신
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

    HandleEvent<WindowCloseEvent>(_event, JAM_BIND_MEMBER_LISTENER(Application::OnWindowCloseEvent_));

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

std::string_view Application::GetApplicationName() const
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
