#include "pch.h"

#include "PointLightComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

Json PointLightComponent::Serialize(const Ref<Scene>& _pScene, Entity _entity) const
{
    Json json;
    json["radiance"]     = radiance;
    json["intensity"]    = intensity;
    json["fallOffStart"] = fallOffStart;
    json["fallOffEnd"]   = fallOffEnd;
    return json;
}

void PointLightComponent::Deserialize(
    const Json& _json,
    Ref<Scene>& _pScene,
    Entity      _entity)
{
    TryParseJsonField(_json, "radiance", radiance);
    TryParseJsonField(_json, "intensity", intensity);
    TryParseJsonField(_json, "fallOffStart", fallOffStart);
    TryParseJsonField(_json, "fallOffEnd", fallOffEnd);
}

void PointLightComponent::DrawEditor(Ref<Scene>& _pScene, Entity _entity)
{
    if (ImGui::BeginTable("PointLightEditor", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // setup table
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("radiance");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::ColorEdit3("##PointlightRadiance", &radiance.x);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("intensity");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##PointlightIntensity", &intensity, 0.1f, 0.f, 10000.f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off start");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##PointlightFallOffStart", &fallOffStart, 0.1f, 0.f, fallOffEnd - k_epsilon);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fall off end");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_DragFloat("##PointlightFallOffEnd", &fallOffEnd, 0.1f, fallOffStart + k_epsilon, 10000.f);

        ImGui::EndTable();
    }
}

}   // namespace jam