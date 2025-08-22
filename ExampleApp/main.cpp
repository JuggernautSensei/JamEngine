#include "pch.h"

#include "DemoScene.h"

class Sandbox final : public Application
{
public:
    explicit Sandbox(const ApplicationCreateInfo& _info)
        : Application(_info)
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
        const Window& window = GetWindow();
        window.ResizeWindow(1280, 720);
        SetVsync(true);

        // editor
        Scope<EditorLayer> pEditorLayer = std::make_unique<EditorLayer>();
        AttachLayer(std::move(pEditorLayer));

        // scene layer
        SceneLayer* pSceneLayer = GetSceneLayer();
        pSceneLayer->AddScene(std::make_unique<DemoScene>("DemoScene"));
        pSceneLayer->ChangeScene("DemoScene");
    }

    void OnDestroy() override
    {
    }
};

Application* CreateApplication(const CommandLineArguments& _args)
{
    // command line arguments can be used here
    for (int i = 0; i < _args.argCount; ++i)
    {
        Log::Info("Argument {}: {}", i, _args.GetArgument(i));
    }

    ApplicationCreateInfo appInfo;
    appInfo.applicationName  = "Example Application";
    appInfo.workingDirectory = std::filesystem::current_path();
    return new Sandbox(appInfo);
}

JAM_MAIN()