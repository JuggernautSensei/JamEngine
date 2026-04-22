#include "pch.h"

#include "SpotLightComponent.h"

#include "Camera.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

void SpotLightComponent::Serialize(
    ArchiveWriter& _writer,
    Scene*,
    Entity) const
{
    _writer.WriteField("radiance", radiance);
    _writer.WriteField("intensity", intensity);
    _writer.WriteField("fallOffStart", fallOffStart);
    _writer.WriteField("fallOffEnd", fallOffEnd);
    _writer.WriteField("innerConeHalfRad", innerConeHalfRad);
    _writer.WriteField("outerConeHalfRad", outerConeHalfRad);
}

void SpotLightComponent::Deserialize(
    ArchiveReader& _reader,
    Scene*,
    Entity)
{
    _reader.ReadField("radiance", radiance);
    _reader.ReadField("intensity", intensity);
    _reader.ReadField("fallOffStart", fallOffStart);
    _reader.ReadField("fallOffEnd", fallOffEnd);
    _reader.ReadField("innerConeHalfRad", innerConeHalfRad);
    _reader.ReadField("outerConeHalfRad", outerConeHalfRad);
}

void SpotLightComponent::DrawEditPanel(
    Scene* _pScene,
    Entity            _entity)
{
    if (BeginLabelValueTable("SpotLightComponent"))
    {
        ImGui::TableNextColumn();
        Widget_Text("radiance", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        ImGui::ColorEdit3("##SpotlightRadiance", &radiance.x);

        ImGui::TableNextColumn();
        Widget_Text("intensity", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##SpotlightIntensity",intensity, 0.01f, 0.f, Limits<float>::kMax);

        ImGui::TableNextColumn();
        Widget_Text("fall off start", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##SpotlightFallOffStart",fallOffStart, 0.01f, 0.f, fallOffEnd, "%.3fm");

        ImGui::TableNextColumn();
        Widget_Text("fall off end", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##SpotlightFallOffEnd",fallOffEnd, 0.01f, fallOffStart, Camera::kMaxZ, "%.3fm");

        ImGui::TableNextColumn();
        Widget_Text("inner cone half rad (deg)", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragAngle("##SpotlightInnerConeHalfRad", innerConeHalfRad, 0.5f, 0.f, 179.f);

        ImGui::TableNextColumn();
        Widget_Text("outer cone half rad (deg)", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragAngle("##SpotlightOuterConeHalfRad", outerConeHalfRad, 0.5f, ToDeg(innerConeHalfRad), 179.f);

        ImGui::EndTable();
    }
}

}   // namespace Jam