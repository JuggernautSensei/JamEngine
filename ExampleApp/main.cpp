#include "pch.h"

#include "DemoScene.h"

class Sandbox final : public jam::Application
{
public:
    explicit Sandbox(const jam::ApplicationCreateInfo& _info)
        : jam::Application(_info)
    {
    }

    ~Sandbox() override                    = default;
    Sandbox(const Sandbox&)                = delete;
    Sandbox& operator=(const Sandbox&)     = delete;
    Sandbox(Sandbox&&) noexcept            = delete;
    Sandbox& operator=(Sandbox&&) noexcept = delete;

private:
    void OnCreate() override
    {
        SceneLayer* pSceneLayer = GetSceneLayer();
        pSceneLayer->AddScene(std::make_unique<DemoScene>("DemoScene"));
        pSceneLayer->ChangeScene("DemoScene");
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
    return new Sandbox(appInfo);
}

JAM_MAIN()