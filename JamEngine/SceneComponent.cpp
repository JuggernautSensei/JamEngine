#include "pch.h"

#include "Scene.h"
#include "SceneComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

void SceneComponent::Serialize(
    ArchiveWriter& _writer,
    Scene*,
    Entity) const
{
    _writer.WriteField("localMatrix", node.GetLocal().GetSRT());
    _writer.WriteField("parent", node.GetParent().GetUID());   // if root parent entity is invalid UID.
}

void SceneComponent::Deserialize(
    ArchiveReader&    _reader,
    Scene* _pScene,
    const Entity      _owner)
{
    // transform
    Mtx4 localMtx = Mtx4::kIdentity;
    if (_reader.ReadField("localTransform", localMtx))
    {
        return;
    }
    node.ModifyLocal().SetSRT(localMtx);

    // parent
    UID parentUid = UID::kNull;
    if (_reader.ReadField("parent", parentUid))
    {
        return;
    }

    // set parent if null uid -> root node
    if (parentUid)
    {
        const Entity parent = _pScene->FindEntityOrNull(parentUid);
        if (!parent)
        {
            JAM_LOG_ERROR("SceneComponent::Deserialize: Parent entity with UID '{}' not found for entity with UID '{}'.", parentUid.GetValue(), _owner.GetUID());
            _reader.MarkError(Error::InvalidData);
            return;
        }

        node.SetParent(parent, true);
    }
}

void SceneComponent::DrawEditPanel(
    Scene* _pScene,
    Entity            _owner)
{
    //if (BeginLabelValueTable("SceneComponent"))
    //{
    //    // coordinate space
    //    enum class eCoordinate
    //    {
    //        World,
    //        Local
    //    };

    //    // static variables
    //    static eCoordinate s_coord = eCoordinate::World;

    //    const bool       bIsWorld  = s_coord == eCoordinate::World;
    //    const Transform& transform = bIsWorld ? node.GetWorld() : node.GetLocal();

    //    // get translation & scale
    //    Vec3 translation = transform.GetTranslation();
    //    Vec3 scale       = transform.GetScale();

    //    // setup
    //    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
    //    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

    //    // translation
    //    {
    //        ImGui::TableNextColumn();
    //        Widget_Text("translation", true);
    //        ImGui::TableNextColumn();
    //        if (Widget_DragTranslation("##TransformTranslation", translation))
    //        {
    //            if (bIsWorld)
    //            {
    //                node.ModifyWorld().SetTranslation(translation);
    //            }
    //            else
    //            {
    //                node.ModifyLocal().SetTranslation(translation);
    //            }
    //        }
    //    }

    //    // rotation
    //    {
    //        static EulerProxy s_proxy = {};
    //        s_proxy.OnBeforeWidget(transform);
    //        Vec3& eulerRad = s_proxy.GetEulerProxy();

    //        ImGui::TableNextColumn();
    //        Widget_Text("rotation", true);
    //        ImGui::TableNextColumn();
    //        if (Widget_DragEuler("##TransformRotation", eulerRad))
    //        {
    //            if (bIsWorld)
    //            {
    //                node.ModifyWorld().SetRotation(eulerRad);
    //            }
    //            else
    //            {
    //                node.ModifyLocal().SetRotation(eulerRad);
    //            }
    //        }

    //        s_proxy.OnAfterWidget();
    //    }

    //    // scale
    //    {
    //        ImGui::TableNextColumn();
    //        Widget_Text("scale", true);
    //        ImGui::TableNextColumn();
    //        if (Widget_DragScale("##TransformScale", scale))
    //        {
    //            if (bIsWorld)
    //            {
    //                node.ModifyWorld().SetScale(scale);
    //            }
    //            else
    //            {
    //                node.ModifyLocal().SetScale(scale);
    //            }
    //        }
    //    }

    //    // space radio buttons
    //    {
    //        ImGui::TableNextColumn();
    //        Widget_Text("mode", true);
    //        ImGui::TableNextColumn();
    //        ImGui::RadioButton("world", reinterpret_cast<int*>(&s_coord), static_cast<int>(eCoordinate::World));
    //        ImGui::SameLine();
    //        ImGui::RadioButton("local", reinterpret_cast<int*>(&s_coord), static_cast<int>(eCoordinate::Local));
    //    }

    //    ImGui::EndTable();
    //}
}

}   // namespace Jam