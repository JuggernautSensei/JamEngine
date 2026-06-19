#pragma once
#include "Entity.h"

namespace Jam
{

class Model;
class Scene;

[[nodiscard]] constexpr ImVec4 ImVec4From(const ImVec2 _vec2, const float _z = 0.f, const float _w = 0.f)
{
    return { _vec2.x, _vec2.y, _z, _w };
}

[[nodiscard]] constexpr ImVec2 ImVec2From(const ImVec4 _vec4)
{
    return ImVec2 { _vec4.x, _vec4.y };
}

[[nodiscard]] constexpr bool IsZeroApprox(const ImVec4 _vec, const float _epsilon = 1e-6f)
{
    return IsZeroApprox(_vec.x) && IsZeroApprox(_vec.y) && IsZeroApprox(_vec.z) && IsZeroApprox(_vec.w);
}

[[nodiscard]] constexpr bool IsTransparent(const ImVec4 _color, const float _threshold = 1e-6f)
{
    return _color.w < _threshold;
}

// math
[[nodiscard]] bool   IsZeroApprox(ImVec2 _vec);
[[nodiscard]] ImVec4 ImVec4FromHex(UInt32 _rgba);
[[nodiscard]] ImVec2 ImVec2From(Vec2i _vec);

// layout
[[nodiscard]] ImVec2 CalcFitButtonSize(Int32 _numButtons = 1, ImVec2 _region = ImVec2 { 0.f, 0.f });
void                 SetNextItemToAvailWidth();
void                 SetCursorScreenPosFromWindowBottom(float _offsetY = 0.f);   // distance from the bottom of window. maybe always positive
void                 SetCursorScreenPosFromWindowRight(float _offsetX = 0.f);    // distance from the right of window. maybe always positive

bool                              SetPathDragDropPayload(StringView _path);
[[nodiscard]] const ImGuiPayload* AcceptPathDragDropPayload();
[[nodiscard]] StringView          ResolvePathDragDropPayload(const ImGuiPayload* _pPayload);

bool                     SetStringDragDropPayload(StringView _payload, StringView _string);
[[nodiscard]] StringView ResolveStringDragDropPayload(const ImGuiPayload* _pPayload);

bool                              SetEntityDragDropPayload(Entity _entity);
[[nodiscard]] const ImGuiPayload* AcceptEntityDragDropPayload();
[[nodiscard]] Entity              ResolveEntityDragDropPayload(const ImGuiPayload* _pPayload);

[[nodiscard]] StringView EditorDisplayNameOf(ImGuiKey _key);

// table
bool BeginLabelValueTable(const char* _label);   // | Label | Value |  2 columns

}   // namespace Jam
