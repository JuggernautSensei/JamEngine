#pragma once
#include "ImGuiUtilities.h"

namespace jam
{

enum class eEditorStyleVar
{
    AcroballAxisLineThickness,
    AcroballAxisLineLengthRatio,
    AcroballAxisHeadRadius,
    AcroballAxisHeadOutlineThickness,
};

enum class eEditorStyleColor
{
    Error,     // red
    Correct,   // green
    Info,      // blue

    RunButtonBg,
    RunButtonBgHovered,
    RunButtonBgActive,

    EditButtonBg,
    EditButtonBgHovered,
    EditButtonBgActive,

    AcroballAxisX,
    AcroballAxisY,
    AcroballAxisZ,

    AcroballAxisXOutline,
    AcroballAxisYOutline,
    AcroballAxisZOutline,

    AcroballBg,
    AcroballBgHovered,
    AcroballBgActive,

    Min = Error,
    Max = AcroballBgActive,
};
JAM_ENUM_STATIC_ASSERT(eEditorStyleColor);

struct EditorStyle
{
    EditorStyle();

    [[nodiscard]] float  GetVar(eEditorStyleVar _var) const;
    [[nodiscard]] ImVec4 GetColor(eEditorStyleColor _color) const;
    void                 SetVar(eEditorStyleVar _var, float _value);
    void                 SetColor(eEditorStyleColor _color, const ImVec4& _value);

    float  vars[NumEnums<eEditorStyleVar>()]     = {};
    ImVec4 colors[NumEnums<eEditorStyleColor>()] = {};
};

}   // namespace jam
