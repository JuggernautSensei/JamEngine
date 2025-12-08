#pragma once

#include "CommandQueue.h"
#include "Timer.h"
#include "WindowsPlatform.h"

namespace jam
{

struct CommandLineArguments
{
    [[nodiscard]] const char* GetArgument(const int _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < numArgs, "Argument index out of range");
        return args[_index];
    }

    Int32  numArgs = 0;
    char** args    = nullptr;
};

struct ApplicationCreateInfo
{
    std::string applicationName  = "jam engine application";
    fs::path    workingDirectory = fs::current_path();
    bool        bEnableEditor    = false;
};

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

    /// @brief 메인 스레드에 커맨드를 제출합니다.
    /// @param _command 제출할 커맨드 (람다 함수)
    /// @note 이 함수는 스레드 세이프합니다.
    /// 해당 함수는 지연처리, 타 스레드의 함수 안전하게 실행 등을 위해 사용됩니다.
    void SubmitCommand(const std::function<void()>& _command);

    /// @brief Vsync를 설정합니다.
    /// @param _bVsync Vsync 활성화 여부
    /// @note vsync(수직 동기화)는 모니터의 재생 빈도에 맞춰 프레임을 출력하여 화면 찢어짐(tearing)을 방지하는 기술입니다.
    void SetVsync(bool _bVsync);

    /// @brief Vsync 활성화 여부를 반환합니다.
    /// @return Vsync 활성화 여부
    [[nodiscard]] bool IsVsyncEnabled() const;

    /// @brief 어플리케이션 데이터 반환
    [[nodiscard]] std::string_view       GetApplicationName() const;
    [[nodiscard]] WindowsPlatform&       GetPlatform();
    [[nodiscard]] const WindowsPlatform& GetPlatform() const;
    [[nodiscard]] bool                   IsEditorEnabled() const;

    /// @brief 타이머 관련
    [[nodiscard]] float GetDeltaTimeSec() const;
    [[nodiscard]] float GetFrameRate() const;

    // project dir
    [[nodiscard]] const fs::path& GetWorkingDirectory() const;
    [[nodiscard]] const fs::path& GetContentsDirectory() const;
    [[nodiscard]] fs::path        JoinWorkingDirectory(const fs::path& _dir) const;
    [[nodiscard]] fs::path        JoinContentsDirectory(const fs::path& _dir) const;

    /// 싱글턴 객체 관리. 임의 호출 금지
    static void Create(const CommandLineArguments& _args);
    static void Destroy();

    /// @brief 어플리케이션 객체의 싱글턴 인스턴스를 반환합니다.
    static [[nodiscard]] Application& GetInstance();

protected:
    explicit Application(const ApplicationCreateInfo& _info);

private:
    void OnWindowCloseEvent_(const WindowCloseEvent& _event);

    /// @brief 어플리케이션이 시작될 때 호출되는 함수
    virtual void OnCreate() {}

    /// @brief 어플리케이션이 종료될 때 호출되는 함수
    virtual void OnDestroy() {}

    std::string     m_applicationName    = {};
    bool            m_bRunning           = false;
    bool            m_bEventDispatchable = false;
    bool            m_bVsync             = false;
    bool            m_bEditorEnabled     = false;
    WindowsPlatform m_window             = {};
    CommandQueue    m_commandQueue       = {};

    // timer
    TickTimer m_timer        = {};
    float     m_deltaTimeSec = 0.f;
    float     m_frameRate    = 0.f;

    // project
    fs::path m_workingDir  = fs::current_path();
    fs::path m_contentsDir = {};

    // 싱글턴 인스턴스
    static Application* s_instance;
};

}   // namespace jam

/// @brief 사용자 정의 어플리케이션 객체를 생성하는 함수
/// @note 해당 함수의 정의는 반드시 프로젝트에서 정의되어야 합니다.
[[nodiscard]] jam::Application* CreateApplication(const jam::CommandLineArguments& _args);
