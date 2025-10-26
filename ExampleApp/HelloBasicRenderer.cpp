#include "pch.h"

#include "HelloBasicRenderer.h"

void HelloBasicRenderer::OnEnter()
{
    BasicRenderer::Init();
}

void HelloBasicRenderer::OnExit()
{
    BasicRenderer::Shutdown();
}

void HelloBasicRenderer::OnUpdate(float _deltaSec)
{
}

void HelloBasicRenderer::OnRender(const SceneView& _view)
{
}

void HelloBasicRenderer::OnImGuiRender()
{
}

void HelloBasicRenderer::OnResize(Int32 _width, Int32 _height)
{
}