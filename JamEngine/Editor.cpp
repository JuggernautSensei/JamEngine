#include "pch.h"

#include "Editor.h"

#include "Entity.h"
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

struct EditorContext
{
    Entity focusEntity                          = Entity::s_null;
    bool   panelVisibility[EnumCount<ePanel>()] = {};
};
static EditorContext g_edtCtx;

///////////////////////////////////////

namespace jam::editor
{

void Init()
{
}

void Shutdown()
{
}

void ImGuiRender()
{
}

void OnEvent(Event& _event)
{
}

void SetPanelVisible(const ePanel _panel, const bool _bVisible)
{
    g_edtCtx.panelVisibility[EnumToInt(_panel)] = _bVisible;
}

bool IsPanelVisible(const ePanel _panel)
{
    return g_edtCtx.panelVisibility[EnumToInt(_panel)];
}

void SetFocusEntity(const Entity _entity)
{
    g_edtCtx.focusEntity = _entity;
}

Entity GetFocusEntity()
{
    return g_edtCtx.focusEntity;
}
}