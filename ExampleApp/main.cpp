#include "pch.h"

class Application final : public jam::Application
{
public:
    explicit Application(const jam::ApplicationCreateInfo& _info)
        : jam::Application(_info)
    {
    }

    ~Application() override = default;

    Application(const Application&)                = delete;
    Application& operator=(const Application&)     = delete;
    Application(Application&&) noexcept            = delete;
    Application& operator=(Application&&) noexcept = delete;

private:
    void OnCreate() override
    {
    }

    void OnDestroy() override
    {
    }
};

jam::Application* CreateApplication(const jam::CommandLineArguments& _args)
{
    // command line arguments can be used here
    for (int i = 0; i < _args.argCount; ++i)
    {
        jam::Log::Info("Argument {}: {}", i, _args.GetArgument(i));
    }

    jam::ApplicationCreateInfo appInfo;
    appInfo.applicationName  = "Example Application";
    appInfo.workingDirectory = std::filesystem::current_path();
    appInfo.targetFrameRate  = 240.f;

    return new Application(appInfo);
}

JAM_MAIN()