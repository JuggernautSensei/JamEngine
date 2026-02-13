#pragma once
#include "Entity.h"

namespace Jam
{

class Model;
class Scene;

[[nodiscard]] constexpr ImVec4 ImVec4From(const ImVec2& _vec2, const float _z = 0.f, const float _w = 0.f)
{
    return { _vec2.x, _vec2.y, _z, _w };
}

[[nodiscard]] constexpr ImVec2 ImVec2From(const ImVec4& _vec4)
{
    return ImVec2 { _vec4.x, _vec4.y };
}

// math
[[nodiscard]] bool   IsZeroApprox(ImVec2 _vec);
[[nodiscard]] ImVec4 ImVec4FromHex(UInt32 _rgba);
[[nodiscard]] ImVec2 ImVec2From(Vec2i _vec);

// layout
[[nodiscard]] ImVec2 CalcFitButtonSize(Int32 _numButtons = 1, ImVec2 _region = ImVec2 { 0.f, 0.f });
void                 SetNextItemToAvailWidth();
[[nodiscard]] ImVec2 GetContentRegionAvailScaled(float _sx, float _sy);

bool                              SetFileDragDropPayload(const Path& _path);
[[nodiscard]] const ImGuiPayload* AcceptFileDragDropPayload();
[[nodiscard]] Path                ResolvePathDragDropPayload(const ImGuiPayload* _pPayload);

bool                              SetEntityDragDropPayload(Entity _entity);
[[nodiscard]] const ImGuiPayload* AcceptEntityDragDropPayload();
[[nodiscard]] Entity              ResolveEntityDragDropPayload(const ImGuiPayload* _pPayload);

[[nodiscard]] StringView EditorDisplayNameOf(ImGuiKey _key);

// table
bool BeginLabelTable(const char* _label);   // | Label | Value |  2 columns

}   // namespace Jam
