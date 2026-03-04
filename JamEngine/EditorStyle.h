#pragma once
#include "ImGuiEngine.h"

namespace Jam
{

enum class eEditorStyleVar
{
    AcroballAxisLineThickness,
    AcroballAxisLineLengthRatio,
    AcroballAxisHeadRadius,
    AcroballAxisHeadOutlineThickness,
};

enum class eEditorShortCut
{
    SaveScene,
    LoadScene,
    CreateEntity,
    DeleteEntity,
    CloneEntity,
    RunGameToggle,
    FileBrowserNavUndo,
    FileBrowserNavRedo,
    FileBrowserNavBack,
    GizmoCursorMode,
    GizmoTranslationMode,
    GizmoRotationMode,
    GizmoScaleMode,
    GizmoOrientToggle,
    GizmoSnapToggle,
    GizmoTempSnapToggle,
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
};

struct EditorStyle
{
    EditorStyle();

    EnumArray<eEditorStyleVar, float>         vars      = {};
    EnumArray<eEditorStyleColor, ImVec4>      colors    = {};
    EnumArray<eEditorShortCut, ImGuiKeyChord> shortCuts = {};
};

}   // namespace Jam
