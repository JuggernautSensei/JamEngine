#include "pch.h"

#include "SpotLightComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

Json SpotLightComponent::Serialize(const Ref<Scene>& _pScene, Entity _entity) const
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

bool SpotLightComponent::Deserialize(const Json& _json, Ref<Scene>& _pScene, Entity _entity)
{
    try
    {
        radiance         = _json.at("radiance").get<Vec3>();
        intensity        = _json.at("intensity").get<float>();
        fallOffStart     = _json.at("fallOffStart").get<float>();
        fallOffEnd       = _json.at("fallOffEnd").get<float>();
        innerConeHalfRad = _json.at("innerConeHalfRad").get<float>();
        outerConeHalfRad = _json.at("outerConeHalfRad").get<float>();
        return true;
    }
    catch (const std::exception& e)
    {
        SetErrorString(JAM_PRETTY_FUNCTION " failed: {}", e.what());
        return false;
    }
}

void SpotLightComponent::DrawEditor(Ref<Scene>& _pScene, Entity _entity)
{
    if (ImGui::BeginTable("SpotLightEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // setup table
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("radiance");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::ColorEdit3("##SpotlightRadiance", &radiance.x);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("intensity");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##SpotlightIntensity", &intensity, 0.1f, 0.f, 10000.f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("fall off start");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##SpotlightFallOffStart", &fallOffStart, 0.1f, 0.f, fallOffEnd - k_epsilon, "%.1f", ImGuiSliderFlags_ClampOnInput);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("fall off end");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##SpotlightFallOffEnd", &fallOffEnd, 0.1f, fallOffStart + k_epsilon, 10000.f, "%.1f", ImGuiSliderFlags_ClampOnInput);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("inner cone half rad (deg)");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragAngle("##SpotlightInnerConeHalfRad", &innerConeHalfRad, 1.f, 0.f, 179.f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("outer cone half rad (deg)");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragAngle("##SpotlightOuterConeHalfRad", &outerConeHalfRad, 1.f, innerConeHalfRad + ToRad(1.f), 179.f);

        ImGui::EndTable();
    }
}

}   // namespace jam