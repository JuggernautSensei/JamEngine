#include "pch.h"

#include "HelloBasicRenderer.h"

void HelloBasicRenderer::OnEnter()
{
    BasicRenderer::Init();

    auto [w, h] = SceneManager::GetSceneResoultion();
    CompileGraph(w, h);
}

void HelloBasicRenderer::OnExit()
{
    BasicRenderer::Shutdown();
}

void HelloBasicRenderer::OnUpdate(float _deltaSec)
{
}

void HelloBasicRenderer::OnRender(const RenderView& _view)
{
}

void HelloBasicRenderer::OnImGuiRender()
{
}

void HelloBasicRenderer::OnResize(Int32 _width, Int32 _height)
{
}

void HelloBasicRenderer::CompileGraph(Int32 _width, Int32 _height)
{
    BasicRenderer::ClearRenderGraph();
}