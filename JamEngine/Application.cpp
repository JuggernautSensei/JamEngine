#include "pch.h"

#include "Application.h"

#include "AssetManager.h"
#include "Gfx.h"
#include "ILayer.h"
#include "ImGuiGlue.h"
#include "Input.h"
#include "Magic.h"
#include "SceneManager.h"
using namespace jam;

namespace
{

void OnNewFailed()
{
    JAM_CRASH("Failed to create application instance. Please check your CreateApplication implementation.");
}

float k_deltaTimeStepTable[EnumCount<eUpdatePolicy>()] = {
    0.f,           // eUpdatePolicy::Unlimited
    1.f / 30.f,    // eUpdatePolicy::Fixed30FPS
    1.f / 60.f,    // eUpdatePolicy::Fixed60FPS
    1.f / 120.f,   // eUpdatePolicy::Fixed120FPS
    1.f / 144.f,   // eUpdatePolicy::Fixed144FPS
    1.f / 240.f,   // eUpdatePolicy::Fixed240FPS
    0.f            // eUpdatePolicy::ManualStep
};

}   // namespace

namespace jam
{

Application* Application::s_instance = nullptr;

void Application::Create(const CommandLineArguments& _args)
{
    JAM_ASSERT(s_instance == nullptr, "Application instance already exists");

    // 로그 초기화
    log::Init();

    // 싱글톤 객체 생성
    s_instance = CreateApplication(_args);

    // 어플리케이션 초기화 루틴
    {
        // 기본 속성 설정
        std::set_new_handler(OnNewFailed);
        std::locale::global(std::locale(""));
        if (std::setlocale(LC_ALL, "") == nullptr)
        {
            log::Warn("Failed to set locale");
        }

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

        // 렌더러 초기화
        gfx::Init();

        // 입력 초기화
        input::Init();

        // 씬 매니저 초기화
        scene_manager::Init();

        // imgui 초기화
        imgui::Init();

        //// editor 모드일 경우 editor layer 생성
        // if (s_instance->m_bEditor)
        //{
        //     s_instance->AttachLayer(std::make_unique<EditorLayer>());
        // }
    }

    // 사용자 정의 어플리케이션 초기화 루틴
    s_instance->OnCreate();
}

void Application::Destroy()
{
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(s_instance->m_bRunning == false, "Application is still running, cannot destroy instance");

    // 사용자 정의 어플리케이션 종료 루틴
    s_instance->OnDestroy();

    // 윈도우 종료
    s_instance->m_window.Shutdown();

    // imgui 종료
    imgui::Shutdown();

    // 씬 매니저 종료
    scene_manager::Shutdown();

    // 렌더러 종료
    gfx::Shutdown();

    // 에셋 매니저 릭 검사
    if (asset_manager::DetectLeak())
        JAM_ASSERT(0, "Resource leak detected! Please check the log for details.");

    // 그래픽스 릭 검사
    if (gfx::DetectLeak())
        JAM_ASSERT(0, "Graphics resource leak detected! Please check the log for details.");

    // 싱글톤 객체 삭제
    delete s_instance;
    s_instance = nullptr;

    // 로그 종료
    log::Shutdown();
}

Application& Application::GetInstance()
{
    JAM_ASSERT(s_instance, "Application instance is null");
    return *s_instance;
}

Application::Application(const ApplicationCreateInfo& _info)
    : m_applicationName(_info.applicationName)
    , m_bRunning(true)
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
    m_directories.cachesDir        = m_directories.workingDir / k_cacheDirName;
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
                scene_manager::Update(deltaSec);
                input::Update();
            }

            // 렌더링
            {
                imgui::BeginFrame();

                // 씬 렌더링
                if (!m_bEditor) // runtime
                {
                    scene_manager::RenderRuntime();
                }

                imgui::EndFrame();

                // 프레임 프레젠트
                gfx::Frame(m_bVsync);
            }
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
    JAM_ASSERT(s_instance, "Application instance is null");
    JAM_ASSERT(_event.IsHandled() == false, "Event '{}' is already handled.", _event.GetName());

#ifdef JAM_DEBUG
    if (_event.GetCategory() != eEventCategory::Input)   // 디버그용 로그
    {
        JAM_LOG_DEBUG("Dispatching event: {}", _event.ToString());
    }
#endif

    m_window.OnEvent(_event);
    gfx::OnEvent(_event);
    scene_manager::OnEvent(_event);
    input::OnEvent(_event);
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

}   // namespace jam
