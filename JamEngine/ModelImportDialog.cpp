#include "pch.h"

#include "ModelImportDialog.h"

#include "AssimpImporter.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "WindowsUtilities.h"
#include "EditorWidgets.h"

#include "Material.h"
#include "Mesh.h"
#include "MessageDialog.h"
#include "Model.h"
#include "VertexLayoutEntry.h"

namespace Jam
{

ModelImportDialog::ModelImportDialog(
    const ModelImportDialogInfo& _info)
    : AssetCreationDialog(_info.baseInfo)
    , m_importPath(_info.importPathOrEmpty)
{
    if (!_info.importPathOrEmpty.empty())
    {
        m_assetSaveDir  = _info.importPathOrEmpty.parent_path();
        m_assetBaseName = BaseNameOf(_info.importPathOrEmpty);
    }

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
        m_saveFd.SetTitle("select save directory");
    }
}

void ModelImportDialog::OnShow()
{
    // render table
    if (BeginLabelTable("ImportImageDialog"))
    {
        // import path
        ImGui::TableNextColumn();
        Widget_Text("import path", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##ModelImportSelector", m_importPath, m_importFd);

        // save directory
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##ModelSaveDirSelector", m_assetSaveDir, m_saveFd);

        // name
        ImGui::TableNextColumn();
        Widget_Text("assset name", true);
        ImGui::TableNextColumn();
        Widget_InputText("##ImageName", m_assetBaseName);

        // vertex layout
        ImGui::TableNextColumn();
        Widget_Text("vertex layout", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pVle);

        // generate normals
        ImGui::TableNextColumn();
        Widget_Text("generate normals", true);
        ImGui::SameLine();
        Widget_HelpMark("generate normals if they do not exist in the model file");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##GenerateNormals", &m_options.bGenerateNormals);

        if (m_options.bGenerateNormals)
        {
            ImGui::Indent();
            ImGui::TableNextColumn();
            Widget_Text("drop existing normals", true);
            ImGui::TableNextColumn();
            ImGui::Checkbox("##DropExistingNormals", &m_options.bDropExistingNormals);
            ImGui::Unindent();
        }

        // compute tangent frame
        ImGui::TableNextColumn();
        Widget_Text("compute tangent frame", true);
        ImGui::SameLine();
        Widget_HelpMark("compute tangent frame if it does not exist in the model file");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##ComputeTangentFrame", &m_options.bGenerateTangentFrame);

        if (m_options.bGenerateTangentFrame)
        {
            ImGui::Indent();
            ImGui::TableNextColumn();
            Widget_Text("drop existing tangent frame", true);
            ImGui::TableNextColumn();
            ImGui::Checkbox("##DropExistingTangentFrame", &m_options.bDropExistingTangentFrame);
            ImGui::Unindent();
        }

        // limit bone weights
        ImGui::TableNextColumn();
        Widget_Text("limit bone weights", true);
        ImGui::TableNextColumn();
        ImGui::Checkbox("##LimitBoneWeights", &m_options.bLimitBondWeights);

        // load optimization for static mesh
        ImGui::TableNextColumn();
        Widget_Text("static mesh optimization", true);
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
    // empty checks
    if (m_assetBaseName.IsEmpty())
    {
        PushEmptyNameErrorMessageDialog_();
        return false;
    }

    // save dir
    if (m_assetSaveDir.empty())
    {
        PushEmptySavePathErrorMessageDialog_();
        return false;
    }

    // vertex layout
    if (!m_pVle)
    {
        PushNullVertexLayoutEntryErrorMessageDialog_();
        return false;
    }

    // import path
    if (m_importPath.empty())
    {
        PushEmptyImportPathErrorMessageDialog_();
        return false;
    }

    // save path
    const Path saveDir = JoinedPath(m_assetSaveDir, m_assetBaseName);
    if (fs::exists(saveDir))
    {
        PushOverwriteConfirmationDialog_(saveDir);
        return false;
    }

    // save asset
    return CreateAndSaveAsset_(saveDir);
}

bool ModelImportDialog::CreateAndSaveAsset_(
    const Path& _path)   // _path is dir
{
    JAM_ASSERT(m_pVle, "vertex layout must be valid here");
    JAM_ASSERT(fs::is_directory(_path), "_path must be directory here");

    // create _path directory
    if (!JAM_EDITOR.CreateDir(_path))
    {
        return false;
    }

    // assimp import
    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile(m_importPath, m_options);
    if (!importer)
    {
        ScratchBuffer tmp;
        tmp.Format("failed to import model file '{}': {}", m_importPath, importer.GetError().ToErrorMessage());

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = tmp.ToStringView();
        return false;
    }

    // instantiate model asset
    const Expect<Ref<Model>> expect = importer->ConvertToModel(m_pVle->GetVertexLayout());
    if (!expect)
    {
        ScratchBuffer tmp;
        tmp.Format("failed to convert imported model data to model asset: {}", expect.GetError().ToErrorMessage());
        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = tmp.ToStringView();
        return false;
    }

    // 모델을 저장하기 위해서는
    // 메쉬, 머테리얼, 텍스처를 각각 별도의 파일로 저장 + 정확한 경로 데이터가 필요함
    const Ref<Model>& pModel = expect.GetValue();

    // process mesh
    {
        // create meshes
        const Path meshDir = _path / "Meshes";
        if (!JAM_EDITOR.CreateDir(meshDir))
        {
            return false;
        }

        // save meshes
        const Vector<Ref<Mesh>>& meshes = pModel->GetMeshes();
        for (const Ref<Mesh>& pMesh: meshes)
        {
            // set virtual path
            const StringView name     = pMesh->GetName();
            Path             meshPath = meshDir / name.GetPtr();
            meshPath.replace_extension(Mesh::k_extension.GetPtr());
            pMesh->SetVirtualPath(meshPath);

            // save mesh asset
            if (!SaveAsset_(meshPath, pMesh))
            {
                return false;
            }
        }
    }

    // process materials & texes
    {
        // 머테리얼을 저장할 때 Texture의 정확한 경로가 필요하므로
        // material을 순회하며 필요한 텍스처를 머저 수집하고
        // 그 후에 텍스처를 저장한 뒤 머테리얼을 저장한다.

        HashSet<Ref<Texture>> textureSet = {};

        // create materials directory
        const Path matDir = _path / "Materials";
        if (!JAM_EDITOR.CreateDir(matDir))
        {
            return false;
        }

        // create texes directory
        const Path texDir = _path / "Textures";
        if (!JAM_EDITOR.CreateDir(texDir))
        {
            return false;
        }

        // gather texes
        const Vector<Ref<Material>>& materials = pModel->GetMaterials();
        for (const Ref<Material>& pMaterial: materials)
        {
            for (eMaterialTextureType type: EnumRanges<eMaterialTextureType>())
            {
                const MaterialTextures& textures = pMaterial->GetTextures();
                if (const Ref<Texture>& pTexture = textures[type])
                {
                    textureSet.emplace(pTexture);
                }
            }
        }

        // save texes
        for (const Ref<Texture>& pTexture: textureSet)
        {
            // set virtual path
            const StringView name    = pTexture->GetName();
            Path             texPath = texDir / name.GetPtr();
            texPath.replace_extension(Texture::k_extension.GetPtr());
            pTexture->SetVirtualPath(texPath);

            // save texture asset
            if (!SaveAsset_(texPath, pTexture))
            {
                return false;
            }
        }

        // save materials
        for (const Ref<Material>& pMaterial: materials)
        {
            // set virtual path
            const StringView name    = pMaterial->GetName();
            Path             matPath = matDir / name.GetPtr();
            matPath.replace_extension(Material::k_extension.GetPtr());
            pMaterial->SetVirtualPath(matPath);

            // save material asset
            if (!SaveAsset_(matPath, pMaterial))
            {
                return false;
            }
        }
    }

    // save model
    // const Path path = ReplacedExtension(_path / m_assetBaseName, Model::k_extension);
    Path path = JoinedPath(_path, m_assetBaseName);
    ReplaceExtension(path, Model::k_extension);
    if (!SaveAsset_(path, pModel))
    {
        return false;
    }

    return true;
}

}   // namespace Jam