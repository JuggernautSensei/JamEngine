#include "pch.h"

#include "DemoScene.h"

DemoScene::DemoScene(const std::string_view& _name)
    : Scene(_name)
{
}

void DemoScene::OnEnter()
{
    Log::Info("DemoScene: OnEnter called");
}

void DemoScene::OnUpdate(float _deltaTime)
{
    Log::Info("DemoScene: OnUpdate called with delta time: {} seconds", _deltaTime);
}

void DemoScene::OnRender()
{
    Log::Info("DemoScene: OnRender called");
}

void DemoScene::OnRenderUI()
{
    Log::Info("DemoScene: OnRenderUI called");
}