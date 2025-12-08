#include "pch.h"

#include "CreatePrimitiveDialog.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "Magic.h"
#include "Mesh.h"
#include "MeshFactory.h"

namespace jam
{

CreatePrimitiveDialog::CreatePrimitiveDialog(const fs::path& _parentPath, const eMeshType _meshType)
    : m_parentPath(_parentPath)
    , m_meshType(_meshType)
{
    JAM_ASSERT(fs::is_directory(_parentPath), "parent path must be a directory.");
}

void CreatePrimitiveDialog::operator()()
{
    switch (m_meshType)
    {
        case eMeshType::Cube:
            DrawCreateCubeTable_();
            break;
        case eMeshType::Sphere:
            DrawCreateSphereTable_();
            break;
        case eMeshType::Plane:
            DrawCreatePlaneTable_();
            break;
        case eMeshType::Circle:
            DrawCreateCircleTable_();
            break;
        case eMeshType::Cone:
            DrawCreateConeTable_();
            break;
        case eMeshType::Cylinder:
            DrawCreateCylinderTable_();
            break;
        default:
            JAM_ASSERT(0, "should never happen.");
    }

    m_popup.Show();
}

void CreatePrimitiveDialog::DrawCreateCubeTable_()
{
    JAM_ASSERT(m_meshType == eMeshType::Cube, "mesh type must be cube.");

    if (ImGui::BeginTable("create cube table", 2, ImGuiTableFlags_SizingFixedFit, k_tableSize))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("name");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##name", &m_buf);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("width");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##width", &m_float0, 0.1f, 1.f, "%.3f"))
            m_float0 = Max(0.01f, m_float0);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("height");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##height", &m_float1, 0.1f, 1.f, "%.3f"))
            m_float1 = Max(0.01f, m_float1);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("depth");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##depth", &m_float2, 0.1f, 1.f, "%.3f"))
            m_float2 = Max(0.01f, m_float2);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_VertexLayoutSelector("##VertexLayoutSelector", m_pEntry);

        ImGui::EndTable();
    }

    const ImVec2 btnRes = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnRes))
    {
        if (OnClickCreateCubeTableOkButton_(m_float0, m_float1, m_float2))
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnRes))
        ImGui::CloseCurrentPopup();
}

void CreatePrimitiveDialog::DrawCreateSphereTable_()
{
    JAM_ASSERT(m_meshType == eMeshType::Sphere, "mesh type must be sphere.");

    if (ImGui::BeginTable("create sphere table", 2, ImGuiTableFlags_SizingFixedFit, k_tableSize))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("name");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##name", &m_buf);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("radius");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##radius", &m_float0, 0.1f, 1.f, "%.3f"))
            m_float0 = Max(0.01f, m_float0);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("subdivisions level");
        ImGui::SameLine();
        Widget_HelpMark("number of times to subdivide the icosahedron faces to increase the sphere's resolution.\n"
                        "higher values result in a smoother sphere but also increase the number of vertices and triangles.\n"
                        "subdivisions level must be between 0 and 6.");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::SliderInt("##SubdivisionLevel", &m_int0, 0, 6, "%d", ImGuiSliderFlags_AlwaysClamp))
            m_int0 = Clamp(m_int0, 0, 6);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_VertexLayoutSelector("##VertexLayoutSelector", m_pEntry);

        ImGui::EndTable();
    }

    const ImVec2 btnRes = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnRes))
    {
        if (OnClickCreateSphereTableOkButton_(m_float0, m_int0))
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnRes))
        ImGui::CloseCurrentPopup();
}

void CreatePrimitiveDialog::DrawCreatePlaneTable_()
{
    JAM_ASSERT(m_meshType == eMeshType::Plane, "mesh type must be plane.");

    if (ImGui::BeginTable("create plane table", 2, ImGuiTableFlags_SizingFixedFit, k_tableSize))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("name");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##name", &m_buf);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("width");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##width", &m_float0, 0.1f, 1.f, "%.3f"))
            m_float0 = Max(0.01f, m_float0);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("depth");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##depth", &m_float1, 0.1f, 1.f, "%.3f"))
            m_float1 = Max(0.01f, m_float1);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_VertexLayoutSelector("##VertexLayoutSelector", m_pEntry);

        ImGui::EndTable();
    }

    const ImVec2 btnRes = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnRes))
    {
        if (OnClickCreatePlaneTableOkButton_(m_float0, m_float1))
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnRes))
        ImGui::CloseCurrentPopup();
}

void CreatePrimitiveDialog::DrawCreateCircleTable_()
{
    JAM_ASSERT(m_meshType == eMeshType::Circle, "mesh type must be circle.");

    if (ImGui::BeginTable("create circle table", 2, ImGuiTableFlags_SizingFixedFit, k_tableSize))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("name");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##name", &m_buf);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("radius");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##radius", &m_float0, 0.1f, 1.f, "%.3f"))
            m_float0 = Max(0.01f, m_float0);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("triangle count");
        ImGui::SameLine();

        Widget_HelpMark("number of triangles that form the circle.\n"
                        "higher values result in a smoother circle.\n"
                        "triangle count must be at least 6.\n"
                        "recommended maximum is 256 to avoid performance issues.");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::SliderInt("##TriangleCount", &m_int0, 6, 256, "%d", ImGuiSliderFlags_AlwaysClamp))
            m_int0 = Max(6, m_int0);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_VertexLayoutSelector("##VertexLayoutSelector", m_pEntry);

        ImGui::EndTable();
    }

    const ImVec2 btnRes = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnRes))
    {
        if (OnClickCreateCircleTableOkButton_(m_float0, m_int0))
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnRes))
        ImGui::CloseCurrentPopup();
}

void CreatePrimitiveDialog::DrawCreateConeTable_()
{
    JAM_ASSERT(m_meshType == eMeshType::Cone, "mesh type must be cone.");

    if (ImGui::BeginTable("create cone table", 2, ImGuiTableFlags_SizingFixedFit, k_tableSize))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("name");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##name", &m_buf);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("height");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##height", &m_float0, 0.1f, 1.f, "%.3f"))
            m_float0 = Max(0.01f, m_float0);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("radius");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##radius", &m_float1, 0.1f, 1.f, "%.3f"))
            m_float1 = Max(0.01f, m_float1);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("bottom triangle count");
        ImGui::SameLine();
        Widget_HelpMark("number of triangles that form the bottom circle of the cone.\n"
                        "higher values result in a smoother cone base.\n"
                        "triangle count must be at least 6.\n"
                        "recommended maximum is 256 to avoid performance issues.");

        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::SliderInt("##bottom_triangle_count", &m_int0, 6, 256, "%d", ImGuiSliderFlags_AlwaysClamp))
            m_int0 = Max(6, m_int0);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_VertexLayoutSelector("##VertexLayoutSelector", m_pEntry);

        ImGui::EndTable();
    }

    const ImVec2 btnRes = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnRes))
    {
        if (OnClickCreateConeTableOkButton_(m_float0, m_float1, m_int0))
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnRes))
        ImGui::CloseCurrentPopup();
}

void CreatePrimitiveDialog::DrawCreateCylinderTable_()
{
    JAM_ASSERT(m_meshType == eMeshType::Cylinder, "mesh type must be cylinder.");

    if (ImGui::BeginTable("create cylinder table", 2, ImGuiTableFlags_SizingFixedFit, k_tableSize))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("name");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##name", &m_buf);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("height");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##height", &m_float0, 0.1f, 1.f, "%.3f"))
            m_float0 = Max(0.01f, m_float0);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("radius");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::InputFloat("##radius", &m_float1, 0.1f, 1.f, "%.3f"))
            m_float1 = Max(0.01f, m_float1);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("bottom triangle count");
        ImGui::SameLine();
        Widget_HelpMark("number of triangles that form the top and bottom circles of the cylinder.\n"
                        "higher values result in a smoother cylinder.\n"
                        "triangle count must be at least 6.\n"
                        "recommended maximum is 256 to avoid performance issues.");

        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        if (ImGui::SliderInt("##bottom_triangle_count", &m_int0, 6, 256, "%d", ImGuiSliderFlags_AlwaysClamp))
            m_int0 = Max(6, m_int0);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FramePaddingAlignedTextUnformatted("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_VertexLayoutSelector("##VertexLayoutSelector", m_pEntry);

        ImGui::EndTable();
    }

    const ImVec2 btnRes = CalcFitButtonSize(2);
    if (ImGui::Button("create", btnRes))
    {
        if (OnClickCreateCylinderTableOkButton_(m_float0, m_float1, m_int0))
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnRes))
        ImGui::CloseCurrentPopup();
}

bool CreatePrimitiveDialog::OnClickCreateCubeTableOkButton_(const float _width, const float _height, const float _depth)
{
    JAM_ASSERT(m_meshType == eMeshType::Cube, "mesh type must be cube.");
    if (m_buf.empty())
    {
        m_popup.Open("error", "mesh name cannot be empty.");
        return false;
    }

    if (!m_pEntry)
    {
        m_popup.Open("error", "vertex layout must be selected.");
        return false;
    }

    if (_width <= 0.f || _height <= 0.f || _depth <= 0.f)
    {
        m_popup.Open("error", "dimensions must be positive values.");
        return false;
    }

    fs::path path = m_parentPath / m_buf;
    path.replace_extension(k_jamMeshExt);
    if (fs::exists(path))
    {
        std::string msg = std::format("cannot create mesh.\ntarget file already exists: {}", path.native());
        m_popup.Open("error", msg);
        return false;
    }

    Mesh mesh = CreateCubeMesh(Vec3 { _width, _height, _depth }, m_pEntry->GetLayout());
    if (!mesh.Save(path))
    {
        std::string msg = std::format("failed to create mesh: {}\nerror: {}", path.native(), GetErrorString());
        m_popup.Open("error", msg);
        return false;
    }

    return true;
}

bool CreatePrimitiveDialog::OnClickCreateSphereTableOkButton_(const float _radius, const Int32 _subdivisionLevel)
{
    JAM_ASSERT(m_meshType == eMeshType::Sphere, "mesh type must be sphere.");

    if (m_buf.empty())
    {
        m_popup.Open("error", "mesh name cannot be empty.");
        return false;
    }

    if (!m_pEntry)
    {
        m_popup.Open("error", "vertex layout must be selected.");
        return false;
    }

    if (_radius <= 0.f)
    {
        m_popup.Open("error", "radius must be a positive value.");
        return false;
    }

    if (_subdivisionLevel < 0 || _subdivisionLevel > 6)
    {
        m_popup.Open("error", "subdivision level must be between 0 and 6.");
        return false;
    }

    fs::path path = m_parentPath / m_buf;
    path.replace_extension(k_jamMeshExt);
    if (fs::exists(path))
    {
        std::string msg = std::format("cannot create mesh.\ntarget file already exists: {}", path.native());
        m_popup.Open("error", msg);
        return false;
    }

    Mesh mesh = CreateSphereMesh(_radius, _subdivisionLevel, m_pEntry->GetLayout());
    if (!mesh.Save(path))
    {
        std::string msg = std::format("failed to create mesh: {}\nerror: {}", path.native(), GetErrorString());
        m_popup.Open("error", msg);
        return false;
    }

    return true;
}

bool CreatePrimitiveDialog::OnClickCreatePlaneTableOkButton_(const float _width, const float _depth)
{
    JAM_ASSERT(m_meshType == eMeshType::Plane, "mesh type must be plane.");

    if (m_buf.empty())
    {
        m_popup.Open("error", "mesh name cannot be empty.");
        return false;
    }

    if (!m_pEntry)
    {
        m_popup.Open("error", "vertex layout must be selected.");
        return false;
    }

    if (_width <= 0.f || _depth <= 0.f)
    {
        m_popup.Open("error", "dimensions must be positive values.");
        return false;
    }

    fs::path path = m_parentPath / m_buf;
    path.replace_extension(k_jamMeshExt);
    if (fs::exists(path))
    {
        std::string msg = std::format("cannot create mesh.\ntarget file already exists: {}", path.native());
        m_popup.Open("error", msg);
        return false;
    }

    Mesh mesh = CreatePlaneMesh(Vec2 { _width, _depth }, m_pEntry->GetLayout());
    if (!mesh.Save(path))
    {
        std::string msg = std::format("failed to create mesh: {}\nerror: {}", path.native(), GetErrorString());
        m_popup.Open("error", msg);
        return false;
    }

    return true;
}

bool CreatePrimitiveDialog::OnClickCreateCircleTableOkButton_(const float _radius, const Int32 _numTri)
{
    JAM_ASSERT(m_meshType == eMeshType::Circle, "mesh type must be circle.");

    if (m_buf.empty())
    {
        m_popup.Open("error", "mesh name cannot be empty.");
        return false;
    }

    if (!m_pEntry)
    {
        m_popup.Open("error", "vertex layout must be selected.");
        return false;
    }

    if (_radius <= 0.f)
    {
        m_popup.Open("error", "radius must be a positive value.");
        return false;
    }

    if (_numTri < 6)
    {
        m_popup.Open("error", "triangle count must be at least 6.");
        return false;
    }

    fs::path path = m_parentPath / m_buf;
    path.replace_extension(k_jamMeshExt);
    if (fs::exists(path))
    {
        std::string msg = std::format("cannot create mesh.\ntarget file already exists: {}", path.native());
        m_popup.Open("error", msg);
        return false;
    }

    Mesh mesh = CreateCircle(_radius, _numTri, m_pEntry->GetLayout());
    if (!mesh.Save(path))
    {
        std::string msg = std::format("failed to create mesh: {}\nerror: {}", path.native(), GetErrorString());
        m_popup.Open("error", msg);
        return false;
    }

    return true;
}

bool CreatePrimitiveDialog::OnClickCreateConeTableOkButton_(const float _height, const float _radius, const Int32 _numBottomTri)
{
    JAM_ASSERT(m_meshType == eMeshType::Cone, "mesh type must be cone.");

    if (m_buf.empty())
    {
        m_popup.Open("error", "mesh name cannot be empty.");
        return false;
    }

    if (!m_pEntry)
    {
        m_popup.Open("error", "vertex layout must be selected.");
        return false;
    }

    if (_height <= 0.f || _radius <= 0.f)
    {
        m_popup.Open("error", "dimensions must be positive values.");
        return false;
    }

    if (_numBottomTri < 6)
    {
        m_popup.Open("error", "triangle count must be at least 6.");
        return false;
    }

    fs::path path = m_parentPath / m_buf;
    path.replace_extension(k_jamMeshExt);
    if (fs::exists(path))
    {
        std::string msg = std::format("cannot create mesh.\ntarget file already exists: {}", path.native());
        m_popup.Open("error", msg);
        return false;
    }

    Mesh mesh = CreateCone(_height, _radius, _numBottomTri, m_pEntry->GetLayout());
    if (!mesh.Save(path))
    {
        std::string msg = std::format("failed to create mesh: {}\nerror: {}", path.native(), GetErrorString());
        m_popup.Open("error", msg);
        return false;
    }

    return true;
}

bool CreatePrimitiveDialog::OnClickCreateCylinderTableOkButton_(const float _height, const float _radius, const Int32 _numBottomTri)
{
    JAM_ASSERT(m_meshType == eMeshType::Cylinder, "mesh type must be cylinder.");

    if (m_buf.empty())
    {
        m_popup.Open("error", "mesh name cannot be empty.");
        return false;
    }

    if (!m_pEntry)
    {
        m_popup.Open("error", "vertex layout must be selected.");
        return false;
    }

    if (_height <= 0.f || _radius <= 0.f)
    {
        m_popup.Open("error", "dimensions must be positive values.");
        return false;
    }

    if (_numBottomTri < 6)
    {
        m_popup.Open("error", "triangle count must be at least 6.");
        return false;
    }

    fs::path path = m_parentPath / m_buf;
    path.replace_extension(k_jamMeshExt);
    if (fs::exists(path))
    {
        std::string msg = std::format("cannot create mesh.\ntarget file already exists: {}", path.native());
        m_popup.Open("error", msg);
        return false;
    }

    Mesh mesh = CreateCylinder(_height, _radius, _numBottomTri, m_pEntry->GetLayout());
    if (!mesh.Save(path))
    {
        std::string msg = std::format("failed to create mesh: {}\nerror: {}", path.native(), GetErrorString());
        m_popup.Open("error", msg);
        return false;
    }

    return true;
}
}   // namespace jam