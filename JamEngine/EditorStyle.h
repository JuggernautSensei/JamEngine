#pragma once
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"

namespace jam
{

enum eEditorStyleColor
{
    eEditorStyleColor_Error,     // red
    eEditorStyleColor_Correct,   // green
    eEditorStyleColor_Info,      // blue

    eEditorStyleColor_RunButtonBg,
    eEditorStyleColor_RunButtonBgHovered,
    eEditorStyleColor_RunButtonBgActive,

    eEditorStyleColor_EditButtonBg,
    eEditorStyleColor_EditButtonBgHovered,
    eEditorStyleColor_EditButtonBgActive,

    eEditorStyleColor_AxisX,
    eEditorStyleColor_AxisY,
    eEditorStyleColor_AxisZ,

    eEditorStyleColor_AcroballBg,
    eEditorStyleColor_AcroballBgHovered,
    eEditorStyleColor_AcroballBgActive,
};
constexpr size_t k_numEditorStyleColors = EnumCount<eEditorStyleColor>();

struct EditorStyle
{
    EditorStyle();

    ImVec4 colors[k_numEditorStyleColors];
};

inline EditorStyle::EditorStyle()
{
    colors[eEditorStyleColor_Error]   = ImVec4FromHex(0xFF5656FF);
    colors[eEditorStyleColor_Correct] = ImVec4FromHex(0x50C878FF);
    colors[eEditorStyleColor_Info]    = ImVec4FromHex(0x4DA8DAFF);

    colors[eEditorStyleColor_AxisX] = ImVec4FromHex(0xF63652FF);
    colors[eEditorStyleColor_AxisY] = ImVec4FromHex(0x70A41CFF);
    colors[eEditorStyleColor_AxisZ] = ImVec4FromHex(0x2F84E3FF);

    // green
    colors[eEditorStyleColor_RunButtonBg]        = ImVec4FromHex(0x388e3cFF);
    colors[eEditorStyleColor_RunButtonBgHovered] = ImVec4FromHex(0x388e3cFF) * 1.15f;
    colors[eEditorStyleColor_RunButtonBgActive]  = ImVec4FromHex(0x388e3cFF) * 1.25f;

    // blue
    colors[eEditorStyleColor_EditButtonBg]        = ImVec4FromHex(0x2772dbFF);
    colors[eEditorStyleColor_EditButtonBgHovered] = ImVec4FromHex(0x2772dbFF) * 1.15f;
    colors[eEditorStyleColor_EditButtonBgActive]  = ImVec4FromHex(0x2772dbFF) * 1.25f;

    colors[eEditorStyleColor_AcroballBg]        = ImVec4 { 1.f, 1.f, 1.f, 0.f };
    colors[eEditorStyleColor_AcroballBgHovered] = ImVec4 { 1.f, 1.f, 1.f, 0.2f };
    colors[eEditorStyleColor_AcroballBgActive]  = ImVec4 { 1.f, 1.f, 1.f, 0.3f };

    // yellow
}

}   // namespace jam
