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
static EditorContext g_ctx;

///////////////////////////////////////

namespace jam::editor
{

void Init()
{
}

void Shutdown()
{
}

void Render()
{
}

void OnEvent(Event& _event)
{
}

void SetPanelVisible(const ePanel _panel, const bool _bVisible)
{
    g_ctx.panelVisibility[EnumToInt(_panel)] = _bVisible;
}

bool IsPanelVisible(const ePanel _panel)
{
    return g_ctx.panelVisibility[EnumToInt(_panel)];
}

void SetFocusEntity(const Entity& _entity)
{
    g_ctx.focusEntity = _entity;
}

Entity GetFocusEntity()
{
    return g_ctx.focusEntity;
}
}