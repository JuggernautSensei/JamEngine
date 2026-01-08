#include "pch.h"

#include "SpotLightComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

void SpotLightComponent::Serialize(
    JsonWriter        _writer,
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
    JsonReader        _reader,
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

void SpotLightComponent::Panel_EditComponent(
    Ref<Scene>& _pScene,
    Entity      _entity)
{
    if (BeginLabelValueTable("SpotLightComponent"))
    {
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("radiance");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        ImGui::ColorEdit3("##SpotlightRadiance", &radiance.x);

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("intensity");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##SpotlightIntensity", &intensity, 0.01, 0.f);

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off start");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##SpotlightFallOffStart", &fallOffStart, 0.01f, 0.f, fallOffEnd, "%.3fm");

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off end");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##SpotlightFallOffEnd", &fallOffEnd, 0.01f, fallOffStart, Camera::k_maxZ, "%.3fm");

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("inner cone half rad (deg)");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragAngle("##SpotlightInnerConeHalfRad", innerConeHalfRad, 1.f, 0.f, 179.f);

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("outer cone half rad (deg)");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragAngle("##SpotlightOuterConeHalfRad", outerConeHalfRad, 1.f, ToDeg(innerConeHalfRad), 179.f);

        ImGui::EndTable()();
    }
}

}   // namespace jam