#include "pch.h"

#include "ModelImportDialog.h"

#include "AssimpImporter.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "WindowsUtilities.h"
#include "EditorWidgets.h"

#include "ImGuiUtilities.h"
#include "Material.h"
#include "Mesh.h"
#include "Model.h"
#include "VertexLayoutEntry.h"

namespace jam
{

ModelImportDialog::ModelImportDialog(
    const fs::path& _importPathOrEmpty)
    : AssetDialog("import model")
    , m_importPath(_importPathOrEmpty)
    , m_importFd(eFileDialogMode::OpenFile)
    , m_selectDirFd(eFileDialogMode::SelectFolder)
{
    // import file dialog
    {
        FileDialogFilter k_filters[] = {
            { "3d models", "*.obj;*.fbx;*.3ds;*.dae;*.ply;*.stl;*.x;*.gltf;*.glb" },
        };

        m_importFd.SetTitle("open model file");
        m_importFd.SetFilters(k_filters);
    }

    // select directory dialog
    {
        m_selectDirFd.SetTitle("select save directory");
    }

    if (!_importPathOrEmpty.empty())
    {
        m_dir  = _importPathOrEmpty.parent_path();
        m_name = _importPathOrEmpty.stem();
    }
}

void ModelImportDialog::OnShow()
{
    // render table
    if (BeginLabelValueTable("ImportImageDialog"))
    {
        // import path
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("import path");
        ImGui::TableNextColumn();
        Widget_PathSelector("##ModelImportSelector", m_importPath, m_importFd);

        // save directory
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("save directory");
        ImGui::TableNextColumn();
        Widget_PathSelector("##ModelSaveDirSelector", m_dir, m_selectDirFd);

        // name
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        ImGui::InputText("##ImageName", &m_name);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("vertex layout");
        ImGui::TableNextColumn();
        SetNextItemAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        // generate normals
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("generate normals");
        ImGui::SameLine();
        Widget_HelpMark("generate normals if they do not exist in the model file");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##GenerateNormals", &m_options.bGenerateNormals);

        if (m_options.bGenerateNormals)
        {
            ImGui::Indent();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("drop existing normals");
            ImGui::TableNextColumn();
            ImGui::Checkbox("##DropExistingNormals", &m_options.bDropExistingNormals);
            ImGui::Unindent();
        }

        // compute tangent frame
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("compute tangent frame");
        ImGui::SameLine();
        Widget_HelpMark("compute tangent frame if it does not exist in the model file");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##ComputeTangentFrame", &m_options.bGenerateTangentFrame);

        if (m_options.bGenerateTangentFrame)
        {
            ImGui::Indent();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("drop existing tangent frame");
            ImGui::TableNextColumn();
            ImGui::Checkbox("##DropExistingTangentFrame", &m_options.bDropExistingTangentFrame);
            ImGui::Unindent();
        }

        // limit bone weights
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("limit bone weights");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##LimitBoneWeights", &m_options.bLimitBondWeights);

        // load optimization for static mesh
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("static mesh optimization");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##StaticMeshOpt", &m_options.bLoadStaticMesh);

        ImGui::EndTable();
    }

    // buttons
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("import", btnSize))
    {
        if (OnClickImportButton_())
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

bool ModelImportDialog::OnClickImportButton_()
{
    if (m_name.empty())
    {
        Editor::PushMessageDialog("error", "model name cannot be empty.");
        return false;
    }

    if (!m_pVle)
    {
        Editor::PushMessageDialog("error", "vertex layout must be selected.");
        return false;
    }

    // m_name 과 같은 이름의 파일이 존재하는지 검사
    const fs::path mdlDir = m_dir / m_name;
    if (fs::exists(mdlDir))
    {
        const StringView msg = GlobalBuffer::FormatTmp("a file or directory with the name '{}' already exists in the target directory.", m_name);
        Editor::PushMessageDialog("error", msg);
        return false;
    }

    // 폴더 생성
    fs::create_directory(mdlDir);

    // assimp import
    AssimpImporter importer = {};
    importer.SetOptions(m_options);
    if (!importer.Import(m_importPath))
    {
        const StringView msg = GlobalBuffer::FormatTmp("failed to import model from file '{}': {}", m_importPath.native(), GlobalBuffer::GetLastError());
        Editor::PushMessageDialog("error", msg);
        return false;
    }

    // 모델 로딩을 위해 필요한 자원들 생성
    const Ref<Model>&            pModel     = importer.CreateModel(m_pVle->GetVertexLayout());
    const Vector<Ref<Mesh>>&     pMeshes    = pModel->GetMeshs();
    const Vector<Ref<Material>>& pMaterials = pModel->GetMaterials();
    HashSet<Ref<ImageTexture>>   textureSet = {};

    // mesh
    if (!pMeshes.empty())
    {
        // create mesh directory
        const fs::path meshDir = mdlDir / "Meshes";
        fs::create_directory(meshDir);

        // save meshes
        for (Ref<Mesh> pMesh: pMeshes)
        {
            // make virtual path
            fs::path meshPath = meshDir / pMesh->GetName().GetPtr();
            meshPath.replace_extension(k_jamMeshExt.GetPtr());
            pMesh->SetVirtualPath(meshPath);

            // save
            if (!pMesh->Save(meshPath, false))
            {
                const StringView msg = GlobalBuffer::FormatTmp("failed to save mesh to disk. {}", GlobalBuffer::GetLastError());
                Editor::PushMessageDialog("error", msg);
                return false;
            }
        }
    }

    // material
    if (!pMaterials.empty())
    {
        // create material directory
        const fs::path matDir = mdlDir / "Materials";
        fs::create_directory(matDir);

        // collect textures
        for (const Ref<Material>& pMat: pMaterials)
        {
            // collect textures (avoid duplicates)
            const MaterialTextures& textures = pMat->GetTextures();
            for (eMaterialTextureType type: eMaterialTextureType {})
            {
                textureSet.emplace(textures.Get(type));
            }
        }

        // save textures
        if (!textureSet.empty())
        {
            // create texture directory
            const fs::path texDir = mdlDir / "ImageTextures";
            fs::create_directory(texDir);

            // save textures
            for (Ref<ImageTexture> pTex: textureSet)
            {
                // make virtual path
                fs::path texPath = texDir / pTex->GetName().GetPtr();
                texPath.replace_extension(k_jamImageTextureExt.GetPtr());
                pTex->SetVirtualPath(texPath);

                // save
                if (!pTex->Save(texPath, false))
                {
                    const StringView msg = GlobalBuffer::FormatTmp("failed to save texture to disk. {}", GlobalBuffer::GetLastError());
                    Editor::PushMessageDialog("error", msg);
                    return false;
                }
            }
        }

        // save materials
        for (Ref<Material> pMat: pMaterials)
        {
            // make virtual path
            fs::path matPath = matDir / pMat->GetName().GetPtr();
            matPath.replace_extension(k_jamMaterialExt.GetPtr());
            pMat->SetVirtualPath(matPath);

            // save
            if (!pMat->Save(matPath, false))
            {
                const StringView msg = GlobalBuffer::FormatTmp("failed to save material to disk. {}", GlobalBuffer::GetLastError());
                Editor::PushMessageDialog("error", msg);
                return false;
            }
        }

        // why collect textures -> save textures -> save materials?
        // 머테리얼 저장시 텍스처의 경로가 필요하기 때문임.
    }

    return SaveAsset(pModel, mdlDir, m_name);
}

}   // namespace jam