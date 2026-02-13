#pragma once

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

    [[nodiscard]] float         GetVar(eEditorStyleVar _var) const;
    [[nodiscard]] ImVec4        GetColor(eEditorStyleColor _color) const;
    [[nodiscard]] ImGuiKeyChord GetShortCut(eEditorShortCut _shortCut) const;
    void                        SetVar(eEditorStyleVar _var, float _value);
    void                        SetColor(eEditorStyleColor _color, const ImVec4& _value);
    void                        SetShortCut(eEditorShortCut _shortCut, ImGuiKeyChord _value);

    float         vars[CountOf<eEditorStyleVar>()]      = {};
    ImVec4        colors[CountOf<eEditorStyleColor>()]  = {};
    ImGuiKeyChord shortCuts[CountOf<eEditorShortCut>()] = {};
};

}   // namespace Jam
