#include "pch.h"

#include "CameraComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"
#include "Scene.h"

namespace jam
{

void CameraComponent::Serialize(
    JsonWriter       _writer,
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    // camera
    JsonWriter cmrWriter = _writer.EnterField("camera");
    cmrWriter.WriteField("projection", camera.GetProjection());
    cmrWriter.WriteField("nearZ", camera.GetNearZ());
    cmrWriter.WriteField("farZ", camera.GetFarZ());
    cmrWriter.WriteField("fovYRad", camera.GetFovYRad());
    cmrWriter.WriteField("width", camera.GetWidth());
    cmrWriter.WriteField("height", camera.GetHeight());

    // enable
    _writer.WriteField("enable", bEnabled);
}

void CameraComponent::Deserialize(
    JsonReader       _reader,
    const Ref<Scene>& _pScene,
    Entity            _owner)
{
    // camera
    if (Optional<JsonReader> cmrReader = _reader.EnterField("camera"))
    {
        eProjection proj    = camera.GetProjection();
        float       nearZ   = camera.GetNearZ();
        float       farZ    = camera.GetFarZ();
        float       fovYRad = camera.GetFovYRad();
        float       width   = camera.GetWidth();
        float       height  = camera.GetHeight();

        cmrReader->ReadField("projection", proj);
        cmrReader->ReadField("nearZ", nearZ);
        cmrReader->ReadField("farZ", farZ);
        cmrReader->ReadField("fovYRad", fovYRad);
        cmrReader->ReadField("width", width);
        cmrReader->ReadField("height", height);

        camera.SetProjection(proj);
        camera.SetNearZ(nearZ);
        camera.SetFarZ(farZ);
        camera.SetFovYRad(fovYRad);
        camera.SetWidth(width);
        camera.SetHeight(height);
    }

    // enable
    _reader.ReadField("enable", bEnabled);
}

void CameraComponent::DrawEditor(
    const Ref<Scene>& _pScene,
    const Entity      _entity)
{
    if (BeginComponentEditTable("CameraComponent"))
    {
        // variables
        eProjection proj        = camera.GetProjection();
        Vec2        nearFarZ    = { camera.GetNearZ(), camera.GetFarZ() };
        float       fovYRad     = camera.GetFovYRad();
        Vec2        widthHeight = { camera.GetWidth(), camera.GetHeight() };

        // projection
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("projection");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_ProjectionCombo("##CameraProjSelector", proj))
        {
            camera.SetProjection(proj);
        }

        // near z
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("near / far z");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_DragFloat2("##CameraNearFarZ", &nearFarZ.x, 0.01f, Camera::k_minZ, Camera::k_maxZ, "%.2fm"))
        {
            camera.SetNearZ(nearFarZ.x);
            camera.SetFarZ(nearFarZ.y);
        }

        // fov y
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("fov y");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_DragAngle("##CameraFovY", fovYRad, 0.5f, Camera::k_minFovYRad, Camera::k_maxFovYRad))
        {
            camera.SetFovYRad(fovYRad);
        }

        // width / height
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("width / height");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (Widget_DragFloat2("##CameraWidthHeight", &widthHeight.x, 0.1f, Camera::k_minWidthHeight, Camera::k_maxWidthHeight, "%.1f"))
        {
            camera.SetWidth(widthHeight.x);
            camera.SetHeight(widthHeight.y);
        }

        // main
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("enable");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::Checkbox("##CameraEnable", &bEnabled);

        ImGui::EndTable()();
    }
}

}   // namespace jam