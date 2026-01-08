#include "pch.h"

#include "PrimitiveCreationDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "Magics.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include "VertexLayoutEntry.h"

namespace jam
{

PrimitiveCreationDialog::PrimitiveCreationDialog(
    const ePrimitiveCreationType _meshType,
    const fs::path&              _dirOrEmpty)
    : AssetDialog("create primitive")
    , m_meshType(_meshType)
    , m_dir(_dirOrEmpty)
    , m_fd(eFileDialogMode::SelectFolder)
{
    if (JAM_DEBUG_ENABLED)
    {
        if (!_dirOrEmpty.empty())
        {
            JAM_ASSERT(fs::is_directory(_dirOrEmpty), "given path is not a directory: {}", _dirOrEmpty.native());
        }
    }
}

void PrimitiveCreationDialog::OnShow()
{
    switch (m_meshType)
    {
        case ePrimitiveCreationType::Cube:
            Panel_CubeCreation_();
            break;

        case ePrimitiveCreationType::Sphere:
            Panel_SphereCreation_();
            break;

        case ePrimitiveCreationType::Plane:
            Panel_PlaneCreation_();
            break;

        case ePrimitiveCreationType::Circle:
            Panel_CircleCreation_();
            break;

        case ePrimitiveCreationType::Cone:
            Panel_ConeCreation_();
            break;

        case ePrimitiveCreationType::Cylinder:
            Panel_CylinderCreation_();
            break;

        default:
            JAM_ASSERT(0, "should never happen.");
    }
}

void PrimitiveCreationDialog::Panel_CubeCreation_()
{
    JAM_ASSERT(m_meshType == ePrimitiveCreationType::Cube, "mesh type must be cube.");

    if (BeginLabelValueTable("ShowCreateCube"))
    {
        // dir
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("directory");
        ImGui::TableNextColumn();
        Widget_PathSelector("##DirectorySelector", m_dir, m_fd);

        // name
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        ImGui::InputText("##name", &m_name);

        // width
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("width");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##width", &m_fParam0, 0.01f, 0.001f);

        // height
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("height");
        SetNextItemAvailWidth();
        Widget_DragFloat("##height", &m_fParam1, 0.01f, 0.001f);

        // depth
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("depth");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##depth", &m_fParam2, 0.01f, 0.001f);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }

    // buttons
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreateButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

void PrimitiveCreationDialog::Panel_SphereCreation_()
{
    JAM_ASSERT(m_meshType == ePrimitiveCreationType::Sphere, "mesh type must be sphere.");

    if (BeginLabelValueTable("ShowCreateSphere"))
    {
        // name
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        ImGui::InputText("##name", &m_name);

        // radius
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("radius");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##radius", &m_fParam0, 0.01f, 0.001f);

        // subdivisions level
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("subdivisions level");
        ImGui::SameLine();
        Widget_HelpMark("number of times to subdivide the icosahedron faces to increase the sphere's resolution.\n"
                        "higher values result in a smoother sphere but also increase the number of vertices and triangles.");

        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragInt("##subdivisionsLevel", &m_iParam, 0.05, 0, 8);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }

    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreateButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

void PrimitiveCreationDialog::Panel_PlaneCreation_()
{
    JAM_ASSERT(m_meshType == ePrimitiveCreationType::Plane, "mesh type must be plane.");

    if (BeginLabelValueTable("ShowCreatePlane"))
    {
        // name
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        ImGui::InputText("##name", &m_name);

        // width
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("width");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##width", &m_fParam0, 0.01f, 0.001f);

        // depth
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("depth");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##depth", &m_fParam1, 0.01f, 0.001f);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }

    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreateButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

void PrimitiveCreationDialog::Panel_CircleCreation_()
{
    JAM_ASSERT(m_meshType == ePrimitiveCreationType::Circle, "mesh type must be circle.");

    if (BeginLabelValueTable("ShowCreateCircle"))
    {
        // name
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        ImGui::InputText("##name", &m_name);

        // radius
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("radius");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##radius", &m_fParam0, 0.01f, 0.001f);

        // triangle count
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("triangle count");
        ImGui::SameLine();
        Widget_HelpMark("number of triangles that form the circle.\n"
                        "higher values result in a smoother circle.");

        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragInt("##TriangleCount", &m_iParam, 3, 256);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }

    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreateButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

void PrimitiveCreationDialog::Panel_ConeCreation_()
{
    JAM_ASSERT(m_meshType == ePrimitiveCreationType::Cone, "mesh type must be cone.");

    if (BeginLabelValueTable("ShowCreateCone"))
    {
        // name
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        ImGui::InputText("##name", &m_name);

        // height
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("height");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##height", &m_fParam0, 0.01f, 0.001f);

        // radius
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("radius");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##radius", &m_fParam1, 0.01f, 0.001f);

        // bottom triangle count
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("bottom triangle count");
        ImGui::SameLine();
        Widget_HelpMark("number of triangles that form the bottom circle of the cone.\n"
                        "higher values result in a smoother cone base.");

        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragInt("##BottomTriangleCount", &m_iParam, 3, 256);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }

    // buttons
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreateButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

void PrimitiveCreationDialog::Panel_CylinderCreation_()
{
    JAM_ASSERT(m_meshType == ePrimitiveCreationType::Cylinder, "mesh type must be cylinder.");

    if (BeginLabelValueTable("ShowCreateCylinder"))
    {
        // name
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        ImGui::InputText("##name", &m_name);

        // height
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("height");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##height", &m_fParam0, 0.01f, 0.001f);

        // radius
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("radius");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragFloat("##radius", &m_fParam1, 0.01f, 0.001f);

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("bottom triangle count");
        ImGui::SameLine();
        Widget_HelpMark("number of triangles that form the top and bottom circles of the cylinder.\n"
                        "higher values result in a smoother cylinder.");

        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_DragInt("##BottomTriangleCount", &m_iParam, 3, 256);

        ImGui::TableNextColumn();
        Widget_FrameAlignedText("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }

    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnSize))
    {
        if (OnClickCreateButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

bool PrimitiveCreationDialog::OnClickCreateButton_() const
{
    // check vertex layout entry
    if (!m_pVle)
    {
        Editor::PushMessageDialog("error", "please select a valid vertex layout.");
        return false;
    }

    // check name
    if (m_name.empty())
    {
        Editor::PushMessageDialog("error", "please enter a valid name.");
        return false;
    }

    // create mesh
    const VertexLayout& vl = m_pVle->GetVertexLayout();

    Ref<Mesh> pMesh = nullptr;
    switch (m_meshType)
    {
        case ePrimitiveCreationType::Cube:
        {
            const Vec3 size { m_fParam0, m_fParam1, m_fParam2 };
            pMesh = MakeRef<Mesh>(CreateCubeMesh(size, vl));
            break;
        }
        case ePrimitiveCreationType::Sphere:
        {
            const float radius      = m_fParam0;
            const Int32 subdivLevel = m_iParam;
            pMesh                   = MakeRef<Mesh>(CreateSphereMesh(radius, subdivLevel, vl));
            break;
        }
        case ePrimitiveCreationType::Plane:
        {
            const Vec2 size { m_fParam0, m_fParam1 };
            pMesh = MakeRef<Mesh>(CreatePlaneMesh(size, vl));
            break;
        }
        case ePrimitiveCreationType::Circle:
        {
            const float radius   = m_fParam0;
            const Int32 triCount = m_iParam;
            pMesh                = MakeRef<Mesh>(CreateCircle(radius, triCount, vl));
            break;
        }
        case ePrimitiveCreationType::Cone:
        {
            const float height   = m_fParam0;
            const float radius   = m_fParam1;
            const Int32 triCount = m_iParam;
            pMesh                = MakeRef<Mesh>(CreateCone(height, radius, triCount, vl));
            break;
        }
        case ePrimitiveCreationType::Cylinder:
        {
            const float height   = m_fParam0;
            const float radius   = m_fParam1;
            const Int32 numTries = m_iParam;
            pMesh                = MakeRef<Mesh>(CreateCylinder(height, radius, numTries, vl));
            break;
        }
        default:
            JAM_ASSERT(0, "should never happen.");
            return false;
    }

    // save mesh
    if (!pMesh->Save(m_dir / m_name, eSaveFileFlags_FailIfExists))
    {
        PushOverwriteConfirmationDialog_(pMesh, m_dir / m_name);
        return false;
    }
}

}   // namespace jam