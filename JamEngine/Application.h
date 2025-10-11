#pragma once

#include "CommandQueue.h"
#include "Timer.h"
#include "Window.h"

namespace jam
{

/// @brief 업데이트 정책 열거형
enum class eUpdatePolicy : Int8
{
    Unlimited = 0,   // 무제한 업데이트
    Fixed30FPS,      // 고정 30FPS 업데이트
    Fixed60FPS,      // 고정 60FPS 업데이트
    Fixed120FPS,     // 고정 120FPS 업데이트
    Fixed144FPS,     // 고정 144FPS 업데이트
    Fixed240FPS,     // 고정 240FPS 업데이트
    ManualStep,      // 수동 스텝 (Step() 함수 호출시마다 1프레임 업데이트)
};

/// @brief 커맨드라인 인자 구조체
struct CommandLineArguments
{
    JAM_NODISCARD std::string_view GetArgument(const int _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < argCount, "Argument index out of range");
        return args[_index];
    }

    int    argCount = 0;
    char** args     = nullptr;
};

/// @brief 어플리케이션 생성 정보
struct ApplicationCreateInfo
{
    std::string applicationName  = "jam engine application";
    fs::path    workingDirectory = fs::current_path();
    bool        bEditor          = false;   // editor 모드로 실행할지 여부
};

/// [working directory]
///    - [contents]
///        - [assets]
///            - [meshes]
///            - [imageTextures]
///            - [materials]
///        - [scenes]
///    - [caches]
struct ProjectDirectories
{
    fs::path workingDir       = fs::current_path();
    fs::path contentsDir      = {};
    fs::path assetsDir        = {};
    fs::path meshesDir        = {};
    fs::path imageTexturesDir = {};
    fs::path materialsDir     = {};
    fs::path scenesDir        = {};
    fs::path cachesDir        = {};
};

/// @brief 어플리케이션 클래스
/// 어플리케이션의 메인 루프를 관리합니다.
/// 이 클래스를 상속받아 사용자 정의 어플리케이션 클래스를 구현해야 합니다.
class Application
{
public:
    virtual ~Application() = default;

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    /// @brief 메인 루프
    /// 절대로 임의로 호출하지 마십시오.
    int Run();

    /// @brief 어플리케이션 종료
    /// 호출시 다음 프레임에 어플리케이션이 종료됩니다.
    void Quit();

    /// @brief 이벤트를 디스패치합니다.
    /// @param _event 디스패치할 이벤트 객체
    void DispatchEvent(Event& _event);

    /// @brief 렌더링 스레드에 커맨드를 제출합니다.
    /// @param _command 제출할 커맨드 (람다 함수)
    /// @note 이 함수는 스레드 세이프합니다.
    /// 해당 함수는 지연처리, 타 스레드의 함수 안전하게 실행 등을 위해 사용됩니다.
    void SubmitCommand(const std::function<void()>& _command);

    /// @brief Vsync를 설정합니다.
    /// @param _bVsync Vsync 활성화 여부
    /// @note vsync(수직 동기화)는 모니터의 재생 빈도에 맞춰 프레임을 출력하여 화면 찢어짐(tearing)을 방지하는 기술입니다.
    void SetVsync(const bool _bVsync) { m_bVsync = _bVsync; }

    /// @brief Vsync 활성화 여부를 반환합니다.
    /// @return Vsync 활성화 여부
    JAM_NODISCARD bool IsVsyncEnabled() const { return m_bVsync; }

    /// @brief 업데이트 정책을 설정합니다.
    /// @param _policy 업데이트 정책
    void SetUpdatePolicy(eUpdatePolicy _policy);

    /// @brief 업데이트 정책을 반환합니다.
    /// @return 업데이트 정책
    JAM_NODISCARD eUpdatePolicy GetUpdatePolicy() const { return m_updatePolicy; }

    /// @brief 수동 스텝 모드에서 1프레임을 진행합니다.
    /// @param _sec 강제로 지정할 델타타임 (기본값: 1/60초)
    /// @note 업데이트 정책이 ManualStep일 때만 동작합니다.
    void Step(float _sec = 1.0f / 60.0f);

    JAM_NODISCARD const ProjectDirectories& GetProjectDirectories() const;
    JAM_NODISCARD std::string_view GetApplicationName() const;
    JAM_NODISCARD const TickTimer& GetTimer() const;
    JAM_NODISCARD const Window&    GetWindow() const;

    /// 싱글턴 객체 관리. 임의 호출 금지
    static void Create(const CommandLineArguments& _args);
    static void Destroy();

    /// @brief 어플리케이션 객체의 싱글턴 인스턴스를 반환합니다.
    static JAM_NODISCARD Application& GetInstance();

protected:
    explicit Application(const ApplicationCreateInfo& _info);

private:
    /// @brief 어플리케이션이 시작될 때 호출되는 함수
    virtual void OnCreate() = 0;

    /// @brief 어플리케이션이 종료될 때 호출되는 함수
    virtual void OnDestroy() = 0;

    std::string        m_applicationName = {};
    bool               m_bRunning        = false;
    bool               m_bVsync          = false;
    bool               m_bEditor         = false;
    Window             m_window          = {};
    CommandQueue       m_cmdQueue        = {};
    ProjectDirectories m_directories     = {};

    // update
    eUpdatePolicy m_updatePolicy        = eUpdatePolicy::Unlimited;
    float         m_manualUpdateStepSec = 1.f / 60.f;
    bool          m_bStepRequested      = false;

    // timer
    TickTimer m_timer           = {};
    float     m_deltaSecCounter = 0.0f;

    // 싱글턴 인스턴스
    static Application* s_instance;
};

}   // namespace jam

/// @brief 사용자 정의 어플리케이션 객체를 생성하는 함수
/// @note 해당 함수의 정의는 반드시 프로젝트에서 정의되어야 합니다.
JAM_NODISCARD jam::Application* CreateApplication(const jam::CommandLineArguments& _args);
