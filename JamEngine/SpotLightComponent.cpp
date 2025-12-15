#include "pch.h"

#include "SpotLightComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

Json SpotLightComponent::Serialize(
    const Ref<Scene>& _pScene,
    Entity            _entity) const
{
    Json json;
    json["radiance"]         = radiance;
    json["intensity"]        = intensity;
    json["fallOffStart"]     = fallOffStart;
    json["fallOffEnd"]       = fallOffEnd;
    json["innerConeHalfRad"] = innerConeHalfRad;
    json["outerConeHalfRad"] = outerConeHalfRad;
    return json;
}

void SpotLightComponent::Deserialize(
    const Json& _json,
    Ref<Scene>& _pScene,
    Entity      _entity)
{
    TryParseJsonField(_json, "radiance", radiance);
    TryParseJsonField(_json, "intensity", intensity);
    TryParseJsonField(_json, "fallOffStart", fallOffStart);
    TryParseJsonField(_json, "fallOffEnd", fallOffEnd);
    TryParseJsonField(_json, "innerConeHalfRad", innerConeHalfRad);
    TryParseJsonField(_json, "outerConeHalfRad", outerConeHalfRad);
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