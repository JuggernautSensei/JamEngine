#include "pch.h"

#include "ExampleApp.h"

#include "HelloCubeScene.h"

ExampleApp::ExampleApp(const ApplicationCreateInfo& _info)
    : Application(_info)
{
}

void ExampleApp::OnCreate()
{
    Log::Info("hello example app!");

    SetVsync(true);
    Window& win = GetWindow();
    win.ResizeClient(1600, 900);

    std::unique_ptr<HelloCubeScene> pScene = std::make_unique<HelloCubeScene>();
    SceneManager::LoadScene(std::move(pScene));
}

void ExampleApp::OnDestroy()
{
    Log::Info("good-bye example app!");
}
