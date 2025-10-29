#include "pch.h"

#include "ExampleApp.h"

#include "HelloBasicRenderer.h"
#include "HelloJamScene.h"
#include "TriangleScene.h"

ExampleApp::ExampleApp(const ApplicationCreateInfo& _info)
    : Application(_info)
{
}

void ExampleApp::OnCreate()
{
    Log::Info("hello example app!");

    SetVsync(true);
    Window& win = GetWindow();
    win.ResizeClient(1600, 900);   // 화면사이즈 1600 x 900

    auto pScene = std::make_unique<HelloJamScene>();
    // auto pScene = std::make_unique<HelloBasicRenderer>();
    // auto pScene = std::make_unique<TriangleScene>();
    SceneManager::LoadScene(std::move(pScene));
}

void ExampleApp::OnDestroy()
{
    Log::Info("good-bye example app!");
}
