#include "pch.h"

#include "CameraComponent.h"

#include "Archive.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "Scene.h"
#include "ImGuiEngine.h"

namespace Jam
{

void CameraComponent::Serialize(
    ArchiveWriter& _writer,
    const Ref<Scene>&,
    Entity) const
{
    _writer.WriteField("camera", camera);
    _writer.WriteField("enable", bEnabled);
}

void CameraComponent::Deserialize(
    ArchiveReader& _reader,
    const Ref<Scene>&,
    Entity)
{
    _reader.ReadField("camera", camera);
    _reader.ReadField("enable", bEnabled);
}

void CameraComponent::DrawEditPanel(
    const Ref<Scene>&,
    Entity)
{
    if (BeginLabelTable("CameraComponent"))
    {
        eProjection proj     = camera.GetProjection();
        Vec2        nearFarZ = { camera.GetNearZ(), camera.GetFarZ() };
        float       fovYRad  = camera.GetFovYRad();
        Vec2        res      = { camera.GetWidth(), camera.GetHeight() };

        // projection
        ImGui::TableNextColumn();
        Widget_Text("projection", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        if (Widget_EnumCombo("##CameraProjSelector", proj))
        {
            camera.SetProjection(proj);
        }

        // near z
        ImGui::TableNextColumn();
        Widget_Text("near / far z", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        if (Widget_DragFloat2("##CameraNearFarZ", &nearFarZ.x, 0.01f, 0.f, 0.f, "%.3fm"))
        {
            camera.SetNearZ(nearFarZ.x);
            camera.SetFarZ(nearFarZ.y);
        }

        // fov y
        ImGui::TableNextColumn();
        Widget_Text("fov y", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        if (Widget_DragAngle("##CameraFovY", fovYRad))
        {
            camera.SetFovYRad(fovYRad);
        }

        // width / height
        ImGui::TableNextColumn();
        Widget_Text("width / height", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        if (Widget_DragFloat2("##CameraWidthHeight", &res.x))
        {
            camera.SetWidth(res.x);
            camera.SetHeight(res.y);
        }

        // main
        ImGui::TableNextColumn();
        Widget_Text("enable", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        ImGui::Checkbox("##CameraEnable", &bEnabled);

        ImGui::EndTable();
    }
}

}   // namespace Jam