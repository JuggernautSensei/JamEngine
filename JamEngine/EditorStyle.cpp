#include "pch.h"

#include "EditorStyle.h"

#include "EditorUtilities.h"

namespace Jam
{

EditorStyle::EditorStyle()
{
    // var
    {
        SetVar(eEditorStyleVar::AcroballAxisHeadRadius, 10.f);
        SetVar(eEditorStyleVar::AcroballAxisHeadOutlineThickness, 2.f);
        SetVar(eEditorStyleVar::AcroballAxisLineThickness, 3.5f);
        SetVar(eEditorStyleVar::AcroballAxisLineLengthRatio, 0.7f);
    }

    // value
    {
        SetColor(eEditorStyleColor::Error, ImVec4FromHex(0xFF5656FF));
        SetColor(eEditorStyleColor::Correct, ImVec4FromHex(0x50C878FF));
        SetColor(eEditorStyleColor::Info, ImVec4FromHex(0x4DA8DAFF));
        SetColor(eEditorStyleColor::AcroballAxisX, ImVec4FromHex(0xF63652FF));
        SetColor(eEditorStyleColor::AcroballAxisY, ImVec4FromHex(0x70A41CFF));
        SetColor(eEditorStyleColor::AcroballAxisZ, ImVec4FromHex(0x2F84E3FF));
        SetColor(eEditorStyleColor::AcroballAxisXOutline, ImVec4FromHex(0xF63652FF) * 1.25f);
        SetColor(eEditorStyleColor::AcroballAxisYOutline, ImVec4FromHex(0x70A41CFF) * 1.25f);
        SetColor(eEditorStyleColor::AcroballAxisZOutline, ImVec4FromHex(0x2F84E3FF) * 1.25f);
        SetColor(eEditorStyleColor::RunButtonBg, ImVec4FromHex(0x388e3cFF));
        SetColor(eEditorStyleColor::RunButtonBgHovered, ImVec4FromHex(0x388e3cFF) * 1.15f);
        SetColor(eEditorStyleColor::RunButtonBgActive, ImVec4FromHex(0x388e3cFF) * 1.25f);
        SetColor(eEditorStyleColor::EditButtonBg, ImVec4FromHex(0x2772dbFF));
        SetColor(eEditorStyleColor::EditButtonBgHovered, ImVec4FromHex(0x2772dbFF) * 1.15f);
        SetColor(eEditorStyleColor::EditButtonBgActive, ImVec4FromHex(0x2772dbFF) * 1.25f);
        SetColor(eEditorStyleColor::AcroballBg, { 1.f, 1.f, 1.f, 0.f });
        SetColor(eEditorStyleColor::AcroballBgHovered, { 1.f, 1.f, 1.f, 0.2f });
        SetColor(eEditorStyleColor::AcroballBgActive, { 1.f, 1.f, 1.f, 0.3f });

        // clamp alpha
        for (ImVec4& col: colors)
        {
            col.w = Clamp(col.w, 0.f, 1.f);
        }
    }

    // eEditorShortCut
    {
        SetShortCut(eEditorShortCut::SaveScene, ImGuiKey_S | ImGuiMod_Ctrl);
        SetShortCut(eEditorShortCut::LoadScene, ImGuiKey_O | ImGuiMod_Ctrl);
        SetShortCut(eEditorShortCut::CreateEntity, ImGuiKey_N | ImGuiMod_Ctrl);
        SetShortCut(eEditorShortCut::DeleteEntity, ImGuiKey_Delete);
        SetShortCut(eEditorShortCut::CloneEntity, ImGuiKey_D | ImGuiMod_Ctrl);
        SetShortCut(eEditorShortCut::RunGameToggle, ImGuiKey_F5);
        SetShortCut(eEditorShortCut::FileBrowserNavUndo, ImGuiKey_MouseX1);
        SetShortCut(eEditorShortCut::FileBrowserNavRedo, ImGuiKey_MouseX2);
        SetShortCut(eEditorShortCut::FileBrowserNavBack, ImGuiKey_LeftAlt | ImGuiKey_LeftArrow);
        SetShortCut(eEditorShortCut::GizmoCursorMode, ImGuiKey_Q);
        SetShortCut(eEditorShortCut::GizmoTranslationMode, ImGuiKey_W);
        SetShortCut(eEditorShortCut::GizmoRotationMode, ImGuiKey_E);
        SetShortCut(eEditorShortCut::GizmoScaleMode, ImGuiKey_R);
        SetShortCut(eEditorShortCut::GizmoOrientToggle, ImGuiKey_T);
        SetShortCut(eEditorShortCut::GizmoSnapToggle, ImGuiKey_Y);
        SetShortCut(eEditorShortCut::GizmoTempSnapToggle, ImGuiKey_LeftCtrl);
    }
}

float EditorStyle::GetVar(
    const eEditorStyleVar _var) const
{
    return vars[UnderlyingFrom(_var)];
}

ImVec4 EditorStyle::GetColor(
    const eEditorStyleColor _color) const
{
    return colors[UnderlyingFrom(_color)];
}

ImGuiKeyChord EditorStyle::GetShortCut(
    const eEditorShortCut _shortCut) const
{
    return shortCuts[UnderlyingFrom(_shortCut)];
}

void EditorStyle::SetVar(
    const eEditorStyleVar _var,
    const float           _value)
{
    vars[UnderlyingFrom(_var)] = _value;
}

void EditorStyle::SetColor(
    const eEditorStyleColor _color,
    const ImVec4&           _value)
{
    colors[UnderlyingFrom(_color)] = _value;
}

void EditorStyle::SetShortCut(
    const eEditorShortCut _shortCut,
    const ImGuiKeyChord   _value)
{
    shortCuts[UnderlyingFrom(_shortCut)] = _value;
}

}   // namespace Jam