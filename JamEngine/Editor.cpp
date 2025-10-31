#include "pch.h"

#include "Editor.h"

#include "Editor_Viewport.h"
#include "Entity.h"

namespace jam
{

struct EditorContext
{
    Editor_Viewport viewportPanel;

    Entity focusEntity = Entity::s_null;

    bool panelVisibility[EnumCount<ePanel>()] = {};

} g_edtCtx;

}   // namespace jam

namespace jam
{

void Editor::Init()
{
    g_edtCtx.viewportPanel.Init();
}

void Editor::Shutdown()
{
    g_edtCtx.viewportPanel.Shutdown();
}

void Editor::Update(const float _deltaSec)
{
    g_edtCtx.viewportPanel.Update(_deltaSec);
}

void Editor::Render()
{
    g_edtCtx.viewportPanel.Render();
}

void Editor::ImGuiRender()
{
    g_edtCtx.viewportPanel.OnImGuiRender();
}

void Editor::OnEvent(Event& _event)
{
}

void Editor::SetPanelVisible(const ePanel _panel, const bool _bVisible)
{
    g_edtCtx.panelVisibility[EnumToInt(_panel)] = _bVisible;
}

bool Editor::IsPanelVisible(const ePanel _panel)
{
    return g_edtCtx.panelVisibility[EnumToInt(_panel)];
}

void Editor::SetFocusEntity(const Entity _entity)
{
    g_edtCtx.focusEntity = _entity;
}

Entity Editor::GetFocusEntity()
{
    return g_edtCtx.focusEntity;
}
}   // namespace jam
