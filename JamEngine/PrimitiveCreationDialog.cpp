#include "pch.h"

#include "PrimitiveCreationDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"

#include "Mesh.h"
#include "MeshFactory.h"
#include "VertexLayoutEntryManager.h"

namespace Jam
{

namespace
{
    FileDialog CreateDirectorySelectorFileDialog_()
    {
        OpenDirectoryDialogInfo info = {};
        info.title                   = "select save directory";
        return FileDialog(info);
    }
}

PrimitiveCreationDialog::PrimitiveCreationDialog(
    const PrimitiveCreationDialogInfo& _desc)
    : AssetCreationDialog(_desc.title, _desc.size)
    , m_type(_desc.type)
    , m_saveFd(CreateDirectorySelectorFileDialog_())
    , m_assetSaveDir(_desc.saveDirOrEmpty)
{
}

void PrimitiveCreationDialog::OnShow()
{
    switch (m_type)
    {
        case ePrimitiveCreation::Cube:
            Panel_CubeCreation_();
            break;

        case ePrimitiveCreation::Sphere:
            Panel_SphereCreation_();
            break;

        case ePrimitiveCreation::Plane:
            Panel_PlaneCreation_();
            break;

        case ePrimitiveCreation::Circle:
            Panel_CircleCreation_();
            break;

        case ePrimitiveCreation::Cone:
            Panel_ConeCreation_();
            break;

        case ePrimitiveCreation::Cylinder:
            Panel_CylinderCreation_();
            break;

        default:
            JAM_UNREACHABLE;
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

void PrimitiveCreationDialog::Panel_CubeCreation_()
{
    JAM_ASSERT(m_type == ePrimitiveCreation::Cube, "mesh type must be cube.");

    if (BeginLabelValueTable("ShowCreateCube"))
    {
        // dir
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##DirectorySelector", m_assetSaveDir, m_saveFd);

        // name
        ImGui::TableNextColumn();
        Widget_Text("asset name", true);
        ImGui::TableNextColumn();
        Widget_InputText("##name", m_assetBaseName);

        // width
        ImGui::TableNextColumn();
        Widget_Text("width", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##width", m_fParam0, 0.01f, 0.001f);

        // height
        ImGui::TableNextColumn();
        Widget_Text("height", true);
        SetNextItemToAvailWidth();
        Widget_DragFloat("##height", m_fParam1, 0.01f, 0.001f);

        // depth
        ImGui::TableNextColumn();
        Widget_Text("depth", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##depth", m_fParam2, 0.01f, 0.001f);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_Text("vertex layout", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }
}

void PrimitiveCreationDialog::Panel_SphereCreation_()
{
    JAM_ASSERT(m_type == ePrimitiveCreation::Sphere, "mesh type must be sphere.");

    if (BeginLabelValueTable("ShowCreateSphere"))
    {
        // dir
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##DirectorySelector", m_assetSaveDir, m_saveFd);

        // name
        ImGui::TableNextColumn();
        Widget_Text("asset name", true);
        ImGui::TableNextColumn();
        Widget_InputText("##name", m_assetBaseName);

        // radius
        ImGui::TableNextColumn();
        Widget_Text("radius", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##radius", m_fParam0, 0.01f, 0.001f);

        // subdivisions level
        ImGui::TableNextColumn();
        Widget_Text("subdivisions level", true);
        ImGui::SameLine();
        Widget_HelpMark("number of times to subdivide the icosahedron faces to increase the sphere's resolution.\n"
                        "higher values result in a smoother sphere but also increase the number of vertices and triangles.");

        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragInt("##subdivisionsLevel", m_iParam, 0.05f, 0, 8);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_Text("vertex layout", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }
}

void PrimitiveCreationDialog::Panel_PlaneCreation_()
{
    JAM_ASSERT(m_type == ePrimitiveCreation::Plane, "mesh type must be plane.");

    if (BeginLabelValueTable("ShowCreatePlane"))
    {
        // dir
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##DirectorySelector", m_assetSaveDir, m_saveFd);

        // name
        ImGui::TableNextColumn();
        Widget_Text("asset name", true);
        ImGui::TableNextColumn();
        Widget_InputText("##name", m_assetBaseName);

        // width
        ImGui::TableNextColumn();
        Widget_Text("width", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##width", m_fParam0, 0.01f, 0.001f);

        // depth
        ImGui::TableNextColumn();
        Widget_Text("depth", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##depth", m_fParam1, 0.01f, 0.001f);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_Text("vertex layout", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }
}

void PrimitiveCreationDialog::Panel_CircleCreation_()
{
    JAM_ASSERT(m_type == ePrimitiveCreation::Circle, "mesh type must be circle.");

    if (BeginLabelValueTable("ShowCreateCircle"))
    {
        // dir
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##DirectorySelector", m_assetSaveDir, m_saveFd);

        // name
        ImGui::TableNextColumn();
        Widget_Text("asset name", true);
        ImGui::TableNextColumn();
        Widget_InputText("##name", m_assetBaseName);

        // radius
        ImGui::TableNextColumn();
        Widget_Text("radius", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##radius", m_fParam0, 0.01f, 0.001f);

        // triangle count
        ImGui::TableNextColumn();
        Widget_Text("triangle count", true);
        ImGui::SameLine();
        Widget_HelpMark("number of triangles that form the circle.\n"
                        "higher values result in a smoother circle.");

        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragInt("##TriangleCount", m_iParam, 3, 256);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_Text("vertex layout", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }
}

void PrimitiveCreationDialog::Panel_ConeCreation_()
{
    JAM_ASSERT(m_type == ePrimitiveCreation::Cone, "mesh type must be cone.");

    if (BeginLabelValueTable("ShowCreateCone"))
    {
        // dir
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##DirectorySelector", m_assetSaveDir, m_saveFd);

        // name
        ImGui::TableNextColumn();
        Widget_Text("asset name", true);
        ImGui::TableNextColumn();
        Widget_InputText("##name", m_assetBaseName);

        // height
        ImGui::TableNextColumn();
        Widget_Text("height", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##height", m_fParam0, 0.01f, 0.001f);

        // radius
        ImGui::TableNextColumn();
        Widget_Text("radius", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##radius", m_fParam1, 0.01f, 0.001f);

        // bottom triangle count
        ImGui::TableNextColumn();
        Widget_Text("bottom triangle count", true);
        ImGui::SameLine();
        Widget_HelpMark("number of triangles that form the bottom circle of the cone.\n"
                        "higher values result in a smoother cone base.");

        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragInt("##BottomTriangleCount", m_iParam, 3, 256);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_Text("vertex layout", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }
}

void PrimitiveCreationDialog::Panel_CylinderCreation_()
{
    JAM_ASSERT(m_type == ePrimitiveCreation::Cylinder, "mesh type must be cylinder.");

    if (BeginLabelValueTable("ShowCreateCylinder"))
    {
        // dir
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##DirectorySelector", m_assetSaveDir, m_saveFd);

        // name
        ImGui::TableNextColumn();
        Widget_Text("asset name", true);
        ImGui::TableNextColumn();
        Widget_InputText("##name", m_assetBaseName);

        // height
        ImGui::TableNextColumn();
        Widget_Text("height", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##height", m_fParam0, 0.01f, 0.001f);

        // radius
        ImGui::TableNextColumn();
        Widget_Text("radius", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragFloat("##radius", m_fParam1, 0.01f, 0.001f);

        ImGui::TableNextColumn();
        Widget_Text("bottom triangle count", true);
        ImGui::SameLine();
        Widget_HelpMark("number of triangles that form the top and bottom circles of the cylinder.\n"
                        "higher values result in a smoother cylinder.");

        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_DragInt("##BottomTriangleCount", m_iParam, 3, 256);

        ImGui::TableNextColumn();
        Widget_Text("vertex layout", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        ImGui::EndTable();
    }
}

bool PrimitiveCreationDialog::OnClickCreateButton_()
{
    // check name
    if (m_assetBaseName.IsEmpty())
    {
        PushEmptyNameErrorMessageDialog_();
        return false;
    }

    // check save dir
    if (m_assetSaveDir.IsEmpty())
    {
        PushEmptySavePathErrorMessageDialog_();
        return false;
    }

    // check vertex layout
    if (m_pVle == nullptr)
    {
        PushNullVertexLayoutEntryErrorMessageDialog_();
        return false;
    }

    String path = String::Joined(m_assetSaveDir, m_assetBaseName, eJoin::File);
    path.ReplaceExtension(Mesh::k_extension);

    if (FileSystem::Exists(path))
    {
        PushOverwriteConfirmationDialog_(path);
        return false;
    }

    // save mesh
    return CreateAndSaveAsset_(path);
}

Ref<Mesh> PrimitiveCreationDialog::CreatePrimitiveMesh_() const
{
    JAM_ASSERT(m_pVle, "vertex layout entry must be valid.");
    const VertexLayout& vl = m_pVle->GetVertexLayout();

    switch (m_type)
    {
        case ePrimitiveCreation::Cube:
        {
            const Vec3 size { m_fParam0, m_fParam1, m_fParam2 };
            return CreateCubeMesh(size, vl);
        }

        case ePrimitiveCreation::Sphere:
        {
            const float radius      = m_fParam0;
            const Int32 subdivLevel = m_iParam;
            return CreateSphereMesh(radius, subdivLevel, vl);
        }

        case ePrimitiveCreation::Plane:
        {
            const Vec2 size { m_fParam0, m_fParam1 };
            return CreatePlaneMesh(size, vl);
        }

        case ePrimitiveCreation::Circle:
        {
            const float radius   = m_fParam0;
            const Int32 triCount = m_iParam;
            return CreateCircle(radius, triCount, vl);
        }

        case ePrimitiveCreation::Cone:
        {
            const float height   = m_fParam0;
            const float radius   = m_fParam1;
            const Int32 triCount = m_iParam;
            return CreateCone(height, radius, triCount, vl);
        }

        case ePrimitiveCreation::Cylinder:
        {
            const float height   = m_fParam0;
            const float radius   = m_fParam1;
            const Int32 numTries = m_iParam;
            return CreateCylinder(height, radius, numTries, vl);
        }

        default:
            JAM_UNREACHABLE;
    }
}

bool PrimitiveCreationDialog::CreateAndSaveAsset_(StringView _path)
{
    const Ref<Mesh> pMesh = CreatePrimitiveMesh_();
    return SaveAsset_(_path, pMesh);
}

}   // namespace Jam