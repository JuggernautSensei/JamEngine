#include "pch.h"

#include "Application.h"

#include "AssetManager.h"
#include "Editor.h"
#include "Graphics.h"
#include "ImGuiGlue.h"
#include "Input.h"
#include "Magic.h"
#include "SceneManager.h"

namespace jam
{

static void OnNewFailed()
{
    JAM_CRASH("Failed to create application instance. Please check your CreateApplication implementation.");
}

constexpr float k_deltaTimeStepTable[EnumCount<eUpdatePolicy>()] = {
    0.f,           // eUpdatePolicy::Unlimited
    1.f / 30.f,    // eUpdatePolicy::Fixed30FPS
    1.f / 60.f,    // eUpdatePolicy::Fixed60FPS
    1.f / 120.f,   // eUpdatePolicy::Fixed120FPS
    1.f / 144.f,   // eUpdatePolicy::Fixed144FPS
    1.f / 240.f,   // eUpdatePolicy::Fixed240FPS
    0.f            // eUpdatePolicy::ManualStep
};

Application* Application::s_instance = nullptr;

void Application::Create(const CommandLineArguments& _args)
{
    JAM_ASSERT(s_instance == nullptr, "Application instance already exists");

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
        ProjectDirectories& directories = s_instance->m_directories;
        fs::create_directory(directories.contentsDir);
        fs::create_directory(directories.scenesDir);
        fs::create_directory(directories.assetsDir);
        fs::create_directory(directories.meshesDir);
        fs::create_directory(directories.imageTexturesDir);
        fs::create_directory(directories.materialsDir);

        // 윈도우 초기화
        s_instance->m_window.Init();
        s_instance->m_window.SetTitle(s_instance->m_applicationName);
        auto [width, height] = s_instance->m_window.GetClientResolution();
        HWND hwnd            = s_instance->m_window.GetPlatformHandle();

        // 렌더러 초기화
        Graphics::Init(hwnd, width, height);

        // 입력 초기화
        Input::Init();

        // 씬 매니저 초기화
        SceneManager::Init();

        // imgui 초기화
        ID3D11Device*        pd3dDevice        = Graphics::GetDevice();
        ID3D11DeviceContext* pd3dDeviceContext = Graphics::GetDeviceContext();
        ImGuiGlue::Init(hwnd, pd3dDevice, pd3dDeviceContext);
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
    Log::Trace("Application '{}' shutdown.", s_instance->m_applicationName);

    // 사용자 정의 어플리케이션 종료 루틴
    s_instance->OnDestroy();

    // 이벤트 디스패치 비활성화
    s_instance->m_bEventDispatchable = false;

    // 윈도우 종료
    s_instance->m_window.Shutdown();

    // imgui 종료
    ImGuiGlue::Shutdown();

    // 씬 매니저 종료
    SceneManager::Shutdown();

    // 렌더러 종료
    Graphics::Shutdown();

    // 에셋 매니저 정리
    AssetManager::ClearAll();

    // 그래픽스 릭 검사
    if (Graphics::DetectLeak())
        JAM_ASSERT(0, "Graphics resource leak detected! Please check the log for details.");

    // 싱글톤 객체 삭제
    delete s_instance;
    s_instance = nullptr;

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
    , m_bEditor(_info.bEditor)
{
    JAM_ASSERT(fs::exists(_info.workingDirectory), "Working directory does not exist: {}", _info.workingDirectory.native());

    m_directories.workingDir       = _info.workingDirectory;
    m_directories.contentsDir      = m_directories.workingDir / k_contentsDirName;
    m_directories.assetsDir        = m_directories.contentsDir / k_assetsDirName;
    m_directories.meshesDir        = m_directories.assetsDir / k_meshesDirName;
    m_directories.imageTexturesDir = m_directories.assetsDir / k_imageTexturesDirName;
    m_directories.materialsDir     = m_directories.assetsDir / k_materialsDirName;
    m_directories.scenesDir        = m_directories.contentsDir / k_scenesDirName;
}

void Application::OnWindowCloseEvent(const WindowCloseEvent& _event)
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
            m_cmdQueue.Execute();   // 이전 프레임에서 수집한 커맨드 실행

            // 델타 타임 계산
            m_timer.Tick();
            float deltaSec = m_timer.GetDelta(eTimeUnit::Sec);

            // 업데이트 정책에 따른 델타 타임 조정
            bool bUpdate = false;
            if (m_updatePolicy == eUpdatePolicy::Unlimited)
            {
                bUpdate = true;   // 무제한 업데이트 모드의 경우 매 프레임마다 업데이트
            }
            else if (m_updatePolicy == eUpdatePolicy::ManualStep)
            {
                // 수동 업데이트 모드의 경우 사용자가 직접 스텝을 요청할 때만 업데이트
                if (m_bStepRequested)
                {
                    JAM_ASSERT(m_manualUpdateStepSec > 0.f, "Manual update step must be greater than 0.");
                    deltaSec         = m_manualUpdateStepSec;
                    bUpdate          = true;
                    m_bStepRequested = false;
                }
            }
            else   // 고정시간 업데이트
            {
                // 고정 프레임 업데이트 모드의 경우 누적된 델타 타임이 설정된 스텝을 초과할 때마다 업데이트
                m_deltaSecCounter += deltaSec;
                float step = k_deltaTimeStepTable[EnumToInt(m_updatePolicy)];
                if (m_deltaSecCounter >= step)
                {
                    m_deltaSecCounter = m_deltaSecCounter - step;   // 남은 시간은 다음 프레임으로 이월
                    bUpdate           = true;
                }
            }

            // 업데이트
            if (bUpdate)
            {
                SceneManager::Update(deltaSec);
                Input::Update();
            }

            // 렌더링
            {
                if (m_bEditor)
                {
                    ImGuiGlue::BeginFrame();
                    SceneManager::ImGuiRender();
                    editor::ImGuiRender();
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
    JAM_ASSERT(_event.IsHandled() == false, "Event '{}' is already handled.", _event.GetName());

    // 어플리케이션이 모두 초기화된 이후 ~ 종료되기 전까지의 이벤트 처리
    if (m_bEventDispatchable == false)
    {
        return;
    }

    // #ifdef JAM_DEBUG
    //     if (_event.GetCategory() != eEventCategory::Input)   // 디버그용 로그
    //         JAM_LOGGER_DEBUG("Dispatching event: {}", _event.ToString());
    // #endif

    HandleEvent<WindowCloseEvent>(_event, JAM_BIND_MEMBER_LISTENER(Application::OnWindowCloseEvent));

    m_window.OnEvent(_event);
    Graphics::OnEvent(_event);
    SceneManager::OnEvent(_event);
    Input::OnEvent(_event);
}

void Application::SubmitCommand(const std::function<void()>& _command)
{
    JAM_ASSERT(_command, "Command is null");
    m_cmdQueue.Submit(_command);
}

void Application::SetUpdatePolicy(const eUpdatePolicy _policy)
{
    m_updatePolicy    = _policy;
    m_deltaSecCounter = 0.f;
}

void Application::Step(const float _sec)
{
    JAM_ASSERT(m_updatePolicy == eUpdatePolicy::ManualStep, "Step can only be called in ManualStep update policy");
    JAM_ASSERT(_sec > 0.f, "Step time must be greater than 0");
    m_manualUpdateStepSec = _sec;
    m_bStepRequested      = true;
}

const ProjectDirectories& Application::GetProjectDirectories() const
{
    return m_directories;
}

std::string_view Application::GetApplicationName() const
{
    return m_applicationName;
}

const TickTimer& Application::GetTimer() const
{
    return m_timer;
}

Window& Application::GetWindow()
{
    return m_window;
}

bool Application::IsEditor() const
{
    return m_bEditor;
}

bool Application::IsRuntime() const
{
    return !m_bEditor;
}

}   // namespace jam
