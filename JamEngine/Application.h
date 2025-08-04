#pragma once

#include "CommandQueue.h"
#include "Config.h"
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

    // core interface
    int  Run();
    void Quit();
    void DispatchEvent(Event& _event);
    void SubmitCommand(const std::function<void()>& _command);

    // layer interface
    ILayer*           PushFrontLayer(std::unique_ptr<ILayer>&& _pLayer);
    ILayer*           PushBackLayer(std::unique_ptr<ILayer>&& _layer);
    void              RemoveLayer(ILayer* _pLayer);
    NODISCARD ILayer* GetLayer(UInt32 _layerHash) const;

    // properties
    void           SetVsync(bool _bVsync);   // enable or disable vsync
    NODISCARD bool IsVsync() const;          // is vsync enabled

    // getter
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
    static void                   Create(const jam::CommandLineArguments& _args);   // public constructor - call at the beginning of your main function
    static void                   Destroy();                                        // public destructor - call at the end of your main function
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
    std::vector<std::unique_ptr<ILayer>> m_layers      = {};        // layers stack
    SceneLayer*                          m_pSceneLayer = nullptr;   // scene layer cache

    // file system
    fs::path m_workingDirectory  = {};
    fs::path m_contentsDirectory = {};   // contents directory
    fs::path m_assetsDirectory   = {};   // assets directory
    fs::path m_modelsDirectory   = {};   // models directory
    fs::path m_texturesDirectory = {};   // textures directory
    fs::path m_scenesDirectory   = {};   // scenes directory

    // singleton instance
    static Application* s_instance;
};

}   // namespace jam

// public API
NODISCARD jam::Application* CreateApplication(const jam::CommandLineArguments& _args);   // you should define this function in your main file
NODISCARD jam::Application& GetApplication();