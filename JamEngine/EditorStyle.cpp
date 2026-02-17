#include "pch.h"

#include "EditorStyle.h"

#include "EditorUtilities.h"

namespace Jam
{

EditorStyle::EditorStyle()
{
    // var
    {
        vars[eEditorStyleVar::AcroballAxisHeadRadius]           = 10.f;
        vars[eEditorStyleVar::AcroballAxisHeadOutlineThickness] = 2.f;
        vars[eEditorStyleVar::AcroballAxisLineThickness]        = 3.5f;
        vars[eEditorStyleVar::AcroballAxisLineLengthRatio]      = 0.7f;
    }

    // value
    {
        colors[eEditorStyleColor::Error]                = ImVec4FromHex(0xFF5656FF);
        colors[eEditorStyleColor::Correct]              = ImVec4FromHex(0x50C878FF);
        colors[eEditorStyleColor::Info]                 = ImVec4FromHex(0x4DA8DAFF);
        colors[eEditorStyleColor::AcroballAxisX]        = ImVec4FromHex(0xF63652FF);
        colors[eEditorStyleColor::AcroballAxisY]        = ImVec4FromHex(0x70A41CFF);
        colors[eEditorStyleColor::AcroballAxisZ]        = ImVec4FromHex(0x2F84E3FF);
        colors[eEditorStyleColor::AcroballAxisXOutline] = ImVec4FromHex(0xF63652FF) * 1.25f;
        colors[eEditorStyleColor::AcroballAxisYOutline] = ImVec4FromHex(0x70A41CFF) * 1.25f;
        colors[eEditorStyleColor::AcroballAxisZOutline] = ImVec4FromHex(0x2F84E3FF) * 1.25f;
        colors[eEditorStyleColor::RunButtonBg]          = ImVec4FromHex(0x388e3cFF);
        colors[eEditorStyleColor::RunButtonBgHovered]   = ImVec4FromHex(0x388e3cFF) * 1.15f;
        colors[eEditorStyleColor::RunButtonBgActive]    = ImVec4FromHex(0x388e3cFF) * 1.25f;
        colors[eEditorStyleColor::EditButtonBg]         = ImVec4FromHex(0x2772dbFF);
        colors[eEditorStyleColor::EditButtonBgHovered]  = ImVec4FromHex(0x2772dbFF) * 1.15f;
        colors[eEditorStyleColor::EditButtonBgActive]   = ImVec4FromHex(0x2772dbFF) * 1.25f;
        colors[eEditorStyleColor::AcroballBg]           = { 1.f, 1.f, 1.f, 0.f };
        colors[eEditorStyleColor::AcroballBgHovered]    = { 1.f, 1.f, 1.f, 0.2f };
        colors[eEditorStyleColor::AcroballBgActive]     = { 1.f, 1.f, 1.f, 0.3f };

        // clamp alpha
        for (ImVec4& col: colors)
        {
            col.w = Clamp(col.w, 0.f, 1.f);
        }
    }

    // eEditorShortCut
    {
        shortCuts[eEditorShortCut::SaveScene]            = ImGuiKey_S | ImGuiMod_Ctrl;
        shortCuts[eEditorShortCut::LoadScene]            = ImGuiKey_O | ImGuiMod_Ctrl;
        shortCuts[eEditorShortCut::CreateEntity]         = ImGuiKey_N | ImGuiMod_Ctrl;
        shortCuts[eEditorShortCut::DeleteEntity]         = ImGuiKey_Delete;
        shortCuts[eEditorShortCut::CloneEntity]          = ImGuiKey_D | ImGuiMod_Ctrl;
        shortCuts[eEditorShortCut::RunGameToggle]        = ImGuiKey_F5;
        shortCuts[eEditorShortCut::FileBrowserNavUndo]   = ImGuiKey_MouseX1;
        shortCuts[eEditorShortCut::FileBrowserNavRedo]   = ImGuiKey_MouseX2;
        shortCuts[eEditorShortCut::FileBrowserNavBack]   = ImGuiKey_LeftAlt | ImGuiKey_LeftArrow;
        shortCuts[eEditorShortCut::GizmoCursorMode]      = ImGuiKey_Q;
        shortCuts[eEditorShortCut::GizmoTranslationMode] = ImGuiKey_W;
        shortCuts[eEditorShortCut::GizmoRotationMode]    = ImGuiKey_E;
        shortCuts[eEditorShortCut::GizmoScaleMode]       = ImGuiKey_R;
        shortCuts[eEditorShortCut::GizmoOrientToggle]    = ImGuiKey_T;
        shortCuts[eEditorShortCut::GizmoSnapToggle]      = ImGuiKey_Y;
        shortCuts[eEditorShortCut::GizmoTempSnapToggle]  = ImGuiKey_LeftCtrl;
    }
}

}   // namespace Jam