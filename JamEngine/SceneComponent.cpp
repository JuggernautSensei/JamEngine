#include "pch.h"

#include "Json.h"
#include "Scene.h"
#include "SceneComponent.h"

#include "EditorUtilities.h"
#include "JsonUtilities.h"

#include "EditorWidgets.h"

namespace jam
{

SceneComponent::SceneComponent()
{
}

SceneComponent::SceneComponent(
    SceneNode&& _node)
    : node(std::move(_node))
{
}

void SceneComponent::Serialize(
    JsonWriter        _writer,
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    // local matrix
    {
        const Mat4 mtx = node.GetLocalTransform().GetMatrix();
        _writer.WriteField("mtx", mtx);
    }

    // parent
    if (node.HasParent())
    {
        const Entity parent = node.GetParent();
        _writer.WriteField("parent", parent.GetUID());
    }
    else
    {
        _writer.WriteField("parent", nullptr);
    }
}

void SceneComponent::Deserialize(
    JsonReader        _reader,
    const Ref<Scene>& _pScene,
    Entity            _owner)
{
    // local matrix
    {
        Mat4 mtx = Mat4::Identity;
        if (_reader.ReadField("mtx", mtx))
        {
            node.ModifyLocalTransform().SetMatrix(mtx);
        }
    }

    // parent
    JsonReader parentReader;
    if (MoveIfHasValue(_reader.EnterField("parent"), parentReader))
    {
        if (!parentReader.IsNull())
        {
            UID parentUid;
            if (parentReader.Read(parentUid))
            {
                if (const Entity parent = _pScene->FindEntity(parentUid))
                {
                    node.SetParent(parent, true);
                }
                else
                {
                    GlobalBuffer::SetLastError("SceneComponent::Deserialize() failed: parent entity with UID {} not found", parentUid);
                    Log::Error(GlobalBuffer::GetLastError());
                    _reader.MarkFailed();
                }
            }
        }
    }
}

void SceneComponent::Panel_EditComponent(
    Ref<Scene>&  _pScene,
    const Entity _entity)
{
    if (BeginLabelValueTable("SceneComponent"))
    {
        // coordinate space
        enum class eCoordinate
        {
            World,
            Local
        };

        // static variables
        static eCoordinate s_coord       = eCoordinate::World;
        static bool        s_bPyrEditing = false;
        static Vec3        s_pyr         = Vec3::Zero;

        const bool       bIsWorld  = s_coord == eCoordinate::World;
        const Transform& transform = bIsWorld ? node.GetWorldTransform() : node.GetLocalTransform();

        // get translation & scale
        Vec3 translation = transform.GetTranslation();
        Vec3 scale       = transform.GetScale();

        // setup
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // translation
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("translation");
            ImGui::TableNextColumn();
            if (Widget_DragTranslation("##TransformTranslation", translation))
            {
                if (bIsWorld)
                {
                    node.ModifyWorldTransform().SetTranslation(translation);
                }
                else
                {
                    node.ModifyLocalTransform().SetTranslation(translation);
                }
            }
        }

        // rotation
        {

            // get rotation (if not editing)
            if (s_bPyrEditing == false)
            {
                s_pyr = transform.ToPitchYawRoll();
            }

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("rotation");
            ImGui::TableNextColumn();
            if (Widget_DragPitchYawRoll("##TransformRotation", s_pyr))
            {
                s_bPyrEditing = true;
                if (bIsWorld)
                {
                    node.ModifyWorldTransform().SetRotation(s_pyr);
                }
                else
                {
                    node.ModifyLocalTransform().SetRotation(s_pyr);
                }
            }

            // editing rotation done
            if (ImGui::IsItemDeactivated())
            {
                s_bPyrEditing = false;
            }
        }

        // scale
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("scale");
            ImGui::TableNextColumn();
            if (Widget_DragScale("##TransformScale", scale))
            {
                if (bIsWorld)
                {
                    node.ModifyWorldTransform().SetScale(scale);
                }
                else
                {
                    node.ModifyLocalTransform().SetScale(scale);
                }
            }
        }

        // space radio buttons
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("mode");
            ImGui::TableNextColumn();
            ImGui::RadioButton("world", reinterpret_cast<Int32*>(s_coord), static_cast<Int32>(eCoordinate::World));
            ImGui::SameLine();
            ImGui::RadioButton("local", reinterpret_cast<Int32*>(s_coord), static_cast<Int32>(eCoordinate::Local));
        }

        ImGui::EndTable();
    }
}

}   // namespace jam