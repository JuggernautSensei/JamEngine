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
    LogSystem::Info("hello example app!");

    // 기본 렌더러 초기화
    BasicRenderer::Init();

    // 수직 동기화 설정
    SetVsync(true);

    // 기본 화면사이즈 1600 x 900
    WindowsPlatform& win = GetPlatform();
    win.ResizeClient(1600, 900);

    // auto pScene = MakeRef<HelloJamScene>();
    // auto pScene = MakeRef<TriangleScene>();
    const Ref<Scene> pScene = MakeRef<HelloBasicRenderer>();
    SceneManager::LoadScene(pScene);
}

void ExampleApp::OnDestroy()
{
    // 기본 렌더러 종료
    BasicRenderer::Shutdown();
    LogSystem::Info("good-bye example app!");
}
