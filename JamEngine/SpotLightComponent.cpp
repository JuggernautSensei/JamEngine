#include "pch.h"

#include "SpotLightComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

void SpotLightComponent::Serialize(
    JsonWriter&       _writer,
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    _writer.WriteField("radiance", radiance);
    _writer.WriteField("intensity", intensity);
    _writer.WriteField("fallOffStart", fallOffStart);
    _writer.WriteField("fallOffEnd", fallOffEnd);
    _writer.WriteField("innerConeHalfRad", innerConeHalfRad);
    _writer.WriteField("outerConeHalfRad", outerConeHalfRad);
}

void SpotLightComponent::Deserialize(
    JsonReader&       _reader,
    const Ref<Scene>& _pScene,
    Entity            _owner)
{
    _reader.ReadField("radiance", radiance);
    _reader.ReadField("intensity", intensity);
    _reader.ReadField("fallOffStart", fallOffStart);
    _reader.ReadField("fallOffEnd", fallOffEnd);
    _reader.ReadField("innerConeHalfRad", innerConeHalfRad);
    _reader.ReadField("outerConeHalfRad", outerConeHalfRad);
}

void SpotLightComponent::DrawEditor(
    Ref<Scene>& _pScene,
    Entity      _entity)
{
    if (ImGui::BeginTable("SpotLightEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // setup table
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("radiance");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::ColorEdit3("##SpotlightRadiance", &radiance.x);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("intensity");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##SpotlightIntensity", &intensity, 0.005f, 0.f, 10.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off start");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##SpotlightFallOffStart", &fallOffStart, 0.05f, 0.f, fallOffEnd - k_epsilon, "%.1f", ImGuiSliderFlags_ClampOnInput);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off end");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##SpotlightFallOffEnd", &fallOffEnd, 0.05f, fallOffStart + k_epsilon, k_floatMax, "%.1f", ImGuiSliderFlags_ClampOnInput);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("inner cone half rad (deg)");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragAngle("##SpotlightInnerConeHalfRad", innerConeHalfRad, 1.f, 0.f, 179.f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("outer cone half rad (deg)");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragAngle("##SpotlightOuterConeHalfRad", outerConeHalfRad, 1.f, innerConeHalfRad + ToRad(1.f), 179.f);

        ImGui::EndTable();
    }
}

}   // namespace jam