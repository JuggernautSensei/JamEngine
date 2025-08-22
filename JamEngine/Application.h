#pragma once

#include "CommandQueue.h"
#include "Event.h"
#include "ILayer.h"
#include "Timer.h"
#include "Window.h"

namespace jam
{
class SceneLayer;

struct CommandLineArguments
{
    NODISCARD std::string_view GetArgument(const int _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < argCount, "Argument index out of range");
        return args[_index];
    }

    int    argCount = 0;
    char** args     = nullptr;
};

struct ApplicationCreateInfo
{
    std::string applicationName  = "jam engine application";
    fs::path    workingDirectory = fs::current_path();
};

// 1. implement CreateApplication, OnCreate, and OnDestroy in your application
// 2. call Application::Create in your main function
// 3. Application::Create -> Application::GetPlatformInstance().Run() -> Application::Destroy()
// this is fixed jam application flow
// if you use built-in main function. You don't have to worry about these flows.
// recommend use JAM_MAIN or JAM_WIN_MAIN to define your main function

class Application
{
public:
    virtual ~Application() = default;

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    // 메인 루프. 절대로 따로 호출하지 마세요.
    int Run();

    // 어플리케이션 종료를 예약합니다. 다음 프레임에 처리됩니다.
    void Quit();

    // 이벤트 객체를 퍼뜨립니다.
    void DispatchEvent(Event& _eventRef);

    // 제출한 커맨드는 지연처리됩니다. 해당 함수는 thread-safe하기에 다른 스레드에서 호출해도 안전합니다
    void SubmitCommand(const std::function<void()>& _command);

    // ILayer 인터페이스
    ILayer*           AttachLayer(Scope<ILayer>&& _pLayer, bool _bAttachAtFront = false /* default is push_back */);
    void              DetachLayer(UInt32 _layerHash);
    NODISCARD ILayer* GetLayer(UInt32 _layerHash) const;

    // about v-sync
    void           SetVsync(const bool _bVsync) { m_bVsync = _bVsync; }
    NODISCARD bool IsVsync() const { return m_bVsync; }   // check if vsync is enabled

    // about log
    void                          SetEventLoggingFilter(const eEventCategoryFlags _flags) { m_eventLoggingFilter = _flags; }
    NODISCARD eEventCategoryFlags GetEventLoggingFilter() const { return m_eventLoggingFilter; }

    // get members
    NODISCARD const Window&    GetWindow() const;
    NODISCARD const TickTimer& GetTimer() const;
    NODISCARD SceneLayer*      GetSceneLayer() const;

    // file system
    NODISCARD const fs::path& GetWorkingDirectory() const { return m_workingDirectory; }     // get working directory
    NODISCARD const fs::path& GetContentsDirectory() const { return m_contentsDirectory; }   // get contents directory
    NODISCARD const fs::path& GetAssetsDirectory() const { return m_assetsDirectory; }       // get assets directory
    NODISCARD const fs::path& GetModelsDirectory() const { return m_modelsDirectory; }       // get models directory
    NODISCARD const fs::path& GetTexturesDirectory() const { return m_texturesDirectory; }   // get textures directory
    NODISCARD const fs::path& GetScenesDirectory() const { return m_scenesDirectory; }       // get scenes directory

    // singletone accessor
    static void                   Create(const CommandLineArguments& _args);   // public constructor - call at the beginning of your main function
    static void                   Destroy();                                   // public destructor - call at the end of your main function
    static NODISCARD Application& GetInstance();

protected:
    explicit Application(const ApplicationCreateInfo& _info);

private:
    // application interface
    virtual void OnCreate()  = 0;   // implement this your application
    virtual void OnDestroy() = 0;   // implement this your application

    std::string  m_applicationName = {};
    bool         m_bRunning        = false;
    bool         m_bVsync          = false;
    Window       m_window          = {};
    TickTimer    m_timer           = {};
    CommandQueue m_commandQueue    = {};

    // layer
    std::vector<Scope<ILayer>> m_layers      = {};        // layers stack
    SceneLayer*                m_pSceneLayer = nullptr;   // scene layer cache

    // file system
    fs::path m_workingDirectory  = {};
    fs::path m_contentsDirectory = {};   // contents directory
    fs::path m_assetsDirectory   = {};   // assets directory
    fs::path m_modelsDirectory   = {};   // models directory
    fs::path m_texturesDirectory = {};   // textures directory
    fs::path m_scenesDirectory   = {};   // scenes directory

    // event (for debugging)
    eEventCategoryFlags m_eventLoggingFilter = eEventCategoryFlags_All & ~eEventCategoryFlags_Input;   // all events except input events

    // singleton instance
    static Application* s_instance;
};

NODISCARD inline Application& GetApplication()
{
    return Application::GetInstance();
}

}   // namespace jam

// public API
NODISCARD jam::Application* CreateApplication(const jam::CommandLineArguments& _args);   // you should define this function in your main file
