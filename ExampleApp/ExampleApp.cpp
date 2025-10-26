#include "pch.h"

#include "ExampleApp.h"

#include "HelloBasicRenderer.h"
#include "HelloCubeScene.h"
#include "HelloTextureScene.h"
#include "HelloTriangleScene.h"

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

    // auto pScene = std::make_unique<HelloTriangleScene>();
    // auto pScene = std::make_unique<HelloCubeScene>();
    // auto pScene = std::make_unique<HelloTextureScene>();
    auto pScene = std::make_unique<HelloBasicRenderer>();
    SceneManager::LoadScene(std::move(pScene));
}

void ExampleApp::OnDestroy()
{
    Log::Info("good-bye example app!");
}
