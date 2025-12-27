#include "pch.h"

#include "ImportModelDialog.h"

#include "AssimpImporter.h"
#include "WindowsUtilities.h"
#include "EditorWidgets.h"

#include "Graphics.h"
#include "ImGuiUtilities.h"
#include "Material.h"
#include "Mesh.h"
#include "Model.h"

namespace jam
{

constexpr FileDialogFilter k_filters[] = {
    {     "3d models", "*.obj;*.fbx;*.3ds;*.dae;*.ply;*.stl;*.x;*.gltf;*.glb" },
    { "native models",                                     k_jamMeshExtFilter },
    {     "all files",                                                  "*.*" }
};

ImportModelDialog::ImportModelDialog(const fs::path& _path)
{
    if (fs::is_directory(_path))
    {
        m_parentDir = _path;
    }
    else
    {
        m_parentDir  = _path.parent_path();
        m_importPath = _path;
    }
}

void ImportModelDialog::operator()()
{
    if (ImGui::BeginTable("ImportMeshDialogTable", 2, ImGuiTableFlags_SizingFixedFit, { 700.f, 0.f }))
    {
        // Setup columns
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // Row 0: Path
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("path");
        ImGui::TableNextColumn();
        Widget_SingleFileDialog("##OpenModelFileDialog", m_importPath, TODO);

        // Row 1: Name
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        ImGui::InputText("##ModelName", &m_buf);

        // Row 2: Vertex Layout
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("vertex vertexLayout");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_VertexLayoutEntryCombo("##VertexLayoutSelector", m_pEntry);

        // Row 3: Compute Tangent
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("compute tangent");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##ComputeTangent", &m_bComputeTangent);

        // Row 4: Normal Generation
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("normal generation");
        ImGui::TableNextColumn();
        SetNextItemWidthToAvailableWidth();
        Widget_NormalGenSelector_(m_normalGenType);

        // Row 5: Drop Normal
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("drop normal");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##DropNormal", &m_bDropNormal);

        // Row 6: Limit Bone Weight
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("limit bone weight");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##LimitBoneWeight", &m_bLimitBoneWeight);

        // Row 7: Static Mesh
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("static mesh");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##StaticMesh", &m_bStaticMesh);

        ImGui::EndTable();
    }

    // Import and Cancel Buttons
    const ImVec2 btnRes = CalcFitButtonSize(2);
    if (ImGui::Button("import", btnRes))
    {
        if (OnClickImportButton_())
            ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnRes))
        ImGui::CloseCurrentPopup();

    m_popup.Show();
}

bool ImportModelDialog::OnClickImportButton_()
{
    // Validation checks
    if (m_importPath.empty())
    {
        m_popup.Open("error", "Import path cannot be empty.");
        return false;
    }

    if (m_buf.empty())
    {
        m_popup.Open("error", "Mesh name cannot be empty.");
        return false;
    }

    if (!m_pEntry)
    {
        m_popup.Open("error", "Vertex vertexLayout must be selected.");
        return false;
    }

    const fs::path dirPath = m_parentDir / m_buf;
    if (fs::exists(dirPath))
    {
        const std::string msg = std::format("A file or directory with the name '{}' already exists in the target directory.", m_buf);
        m_popup.Open("error", msg);
        return false;
    }

    // assimp import
    AssimpImporter importer;
    {
        importer.SetPath(m_importPath);
        importer.ComputeNormal(m_normalGenType);
        if (m_bComputeTangent)
            importer.GenerateTangentFrame(TODO);
        if (m_bDropNormal)
            importer.DropExistingNormal();
        if (m_bLimitBoneWeight)
            importer.LimitBoneWeight();
        if (m_bStaticMesh)
            importer.LoadOptimizationForStaticMesh();

        if (!importer.Import())
        {
            const std::string msg = std::format("Failed to import model from file '{}': {}", m_importPath.native(), GlobalBuffer::GetLastError());
            m_popup.Open("error", msg);
            return false;
        }
    }

    // create output directory
    try
    {
        fs::create_directories(dirPath);
    }
    catch (const fs::filesystem_error& ec)
    {
        const std::string msg = std::format("Failed to create directory '{}': {}", dirPath.native(), ec.what());
        m_popup.Open("error", msg);
        return false;
    }

    // model creation
    if (const Optional<Model> optModel = CreateModelFromImportedModelRootNode_(importer, dirPath))
    {
        const Model& model     = optModel.value();
        fs::path     modelPath = dirPath / m_buf;
        modelPath.replace_extension(k_jamModelExt);
        if (model.Save(modelPath))
        {
            return true;
        }
        else
        {
            const std::string msg = std::format("Failed to save model to disk. {}", GlobalBuffer::GetLastError());
            m_popup.Open("error", msg);
            return false;
        }
    }
    else
    {
        const std::string msg = std::format("Failed to create model from imported data. {}", GlobalBuffer::GetLastError());
        m_popup.Open("error", msg);
        return false;
    }
}

bool ImportModelDialog::Widget_NormalGenSelector_(eNormalGenType& _type) const
{
    static constexpr const char* k_normalGenNames[] = {
        "none",
        "flat",
        "smooth"
    };

    bool bChanged = false;
    if (ImGui::BeginCombo("##NormalGenType", k_normalGenNames[IntFrom(_type)], ImGuiComboFlags_NoArrowButton))
    {
        for (const eNormalGenType type: EnumRanges<eNormalGenType>())
        {
            const bool bIsSelected = (type == _type);
            if (ImGui::Selectable(k_normalGenNames[IntFrom(type)], bIsSelected))
            {
                _type    = type;
                bChanged = true;
            }
        }
        ImGui::EndCombo();
    }
    return bChanged;
}

void ImportModelDialog::CopyVec3Attribute_(
    Vector<Byte>&       _vertData,
    const Vector<Vec3>& _srcData,
    const Int32              _numVerts,
    const Int32              _vertexStride,
    const Int32              _offset,
    const Int32              _byteWidth)
{
    if (std::cmp_equal(_srcData.size(), _numVerts))
    {
        for (Int32 i = 0; i < _numVerts; ++i)
        {
            Byte*       pDst = _vertData.data() + i * _vertexStride + _offset;
            const Vec3* pSrc = &_srcData[i];
            std::memcpy(pDst, pSrc, _byteWidth);
        }
    }
}

void ImportModelDialog::CopyVec4Attribute_(
    Vector<Byte>&       _vertData,
    const Vector<Vec4>& _srcData,
    const Int32              _numVerts,
    const Int32              _vertexStride,
    const Int32              _offset,
    const Int32              _byteWidth)
{
    if (std::cmp_equal(_srcData.size(), _numVerts))
    {
        for (Int32 i = 0; i < _numVerts; ++i)
        {
            Byte*       pDst = _vertData.data() + i * _vertexStride + _offset;
            const Vec4* pSrc = &_srcData[i];
            std::memcpy(pDst, pSrc, _byteWidth);
        }
    }
}

Optional<Mesh> ImportModelDialog::CreateMeshFromImported_(
    const ImportedMesh& _mesh,
    const VertexLayout& _layout)
{
    const Int32 numVerts     = NarrowCast<Int32>(_mesh.positions.size());
    const Int32 vertexStride = _layout.GetStride();

    if (numVerts == 0 || vertexStride == 0)
    {
        GlobalBuffer::SetLastError("Invalid mesh data: numVerts={}, stride={}", numVerts, vertexStride);
        return k_nullopt;
    }

    Vector<Byte> vertsData(numVerts * vertexStride);

    for (const VertexLayoutElement& elem: _layout.GetElements())
    {
        switch (elem.attrib)
        {
            case eVertexAttribute::Position:
                CopyVec3Attribute_(vertsData, _mesh.positions, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::Normal:
                CopyVec3Attribute_(vertsData, _mesh.normals, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::Tangent:
                CopyVec3Attribute_(vertsData, _mesh.tangents, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::Bitangent:
                CopyVec3Attribute_(vertsData, _mesh.bitangents, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::TexCoord0:
                CopyVec3Attribute_(vertsData, _mesh.texCoords0, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::TexCoord1:
                CopyVec3Attribute_(vertsData, _mesh.texCoords1, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::TexCoord2:
                CopyVec3Attribute_(vertsData, _mesh.texCoords2, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::TexCoord3:
                CopyVec3Attribute_(vertsData, _mesh.texCoords3, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::Color0:
                CopyVec4Attribute_(vertsData, _mesh.colors0, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::Color1:
                CopyVec4Attribute_(vertsData, _mesh.colors1, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::Color2:
                CopyVec4Attribute_(vertsData, _mesh.colors2, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            case eVertexAttribute::Color3:
                CopyVec4Attribute_(vertsData, _mesh.colors3, numVerts, vertexStride, elem.offset, elem.byteWidth);
                break;
            default:
                break;
        }
    }

    Mesh mesh;
    mesh.LoadMemory(vertsData, _layout, _mesh.idxData, _mesh.bUseU32);
    return mesh;
}

Ref<ImageTexture> ImportModelDialog::CreateImageTextureFromImportedTexture_(
    const ImportedTexture& _tex,
    const fs::path&        _virtualPath)
{
    const eImageFormat format = _tex.bIsCompressed ? _tex.format : eImageFormat::PNG;
    if (format == eImageFormat::Unknown)
    {
        Log::Warn("Unknown texture format, skipping");
        return nullptr;
    }

    Ref<ImageTexture> pImageTexture = MakeRef<ImageTexture>();
    pImageTexture->LoadMemory(_tex.pixels, format, _virtualPath);
    return pImageTexture;
}

fs::path ImportModelDialog::GenerateTexturePath_(
    const fs::path&        _outputDir,
    const size_t           _index,
    const ImportedTexture& _tex)
{
    const std::string texName = std::format("texture_{}", _index);
    fs::path          path    = _outputDir / texName;

    if (_tex.bIsCompressed && _tex.format != eImageFormat::Unknown)
    {
        path.replace_extension(std::format(".{}", StringFrom(_tex.format)));
    }
    else
    {
        path.replace_extension(".png");
    }

    return path;
}

Optional<Material> ImportModelDialog::CreateMaterialFromImported_(
    const ImportedMaterial&               _importedMat,
    const Vector<Ref<ImageTexture>>& _textures)
{
    Material mat;
    mat.LoadEmpty();

    // Set material properties
    mat.SetAmbientColor(_importedMat.ambientColor);
    mat.SetDiffuseColor(_importedMat.diffuseColor);
    mat.SetSpecularColor(_importedMat.specularColor);
    mat.SetShininess(_importedMat.shininess);
    mat.SetAlbedoColor(_importedMat.albedoColor);
    mat.SetRoughness(_importedMat.roughness);
    mat.SetMetallic(_importedMat.metallic);
    mat.SetEmissiveColor(_importedMat.emissiveColor);
    mat.SetEmissiveIntensity(_importedMat.emissiveIntensity);
    mat.SetDoubleSide(_importedMat.bDoubleSided);
    mat.SetMetallicTextureChannel(_importedMat.metallicTexChannel);
    mat.SetRoughnessTextureChannel(_importedMat.roughnessTexChannel);
    mat.SetFlipYNormalTexture(true);   // normally normal map is based on OpenGL uv coordinate system

    // Set texture maps
    if (_importedMat.albedoTexIdx != k_invalidImportedIdx)
    {
        const Int32 idx = _importedMat.albedoTexIdx;
        if (idx >= 0 && idx < static_cast<Int32>(_textures.size()))
        {
            if (const Ref<ImageTexture>& pTex = _textures[idx])
            {
                mat.SetAlbedoTexture(pTex);
            }
        }
    }

    if (_importedMat.normalTexIdx != k_invalidImportedIdx)
    {
        const Int32 idx = _importedMat.normalTexIdx;
        if (idx >= 0 && idx < static_cast<Int32>(_textures.size()))
        {
            if (const Ref<ImageTexture>& pTex = _textures[idx])
            {
                mat.SetNormalTexture(pTex);
            }
        }
    }

    if (_importedMat.metallicTexIdx != k_invalidImportedIdx)
    {
        const Int32 idx = _importedMat.metallicTexIdx;
        if (idx >= 0 && idx < static_cast<Int32>(_textures.size()))
        {
            if (const Ref<ImageTexture>& pTex = _textures[idx])
            {
                mat.SetMetallicTexture(pTex);
            }
        }
    }

    if (_importedMat.roughnessTexIdx != k_invalidImportedIdx)
    {
        const Int32 idx = _importedMat.roughnessTexIdx;
        if (idx >= 0 && idx < static_cast<Int32>(_textures.size()))
        {
            if (const Ref<ImageTexture>& pTex = _textures[idx])
            {
                mat.SetRoughnessTexture(pTex);
            }
        }
    }

    if (_importedMat.aoTexIdx != k_invalidImportedIdx)
    {
        const Int32 idx = _importedMat.aoTexIdx;
        if (idx >= 0 && idx < static_cast<Int32>(_textures.size()))
        {
            if (const Ref<ImageTexture>& pTex = _textures[idx])
            {
                mat.SetAoTexture(pTex);
            }
        }
    }

    if (_importedMat.emissiveTexIdx != k_invalidImportedIdx)
    {
        const Int32 idx = _importedMat.emissiveTexIdx;
        if (idx >= 0 && idx < static_cast<Int32>(_textures.size()))
        {
            if (const Ref<ImageTexture>& pTex = _textures[idx])
            {
                mat.SetEmissiveTexture(pTex);
            }
        }
    }

    if (_importedMat.lightTexIdx != k_invalidImportedIdx)
    {
        const Int32 idx = _importedMat.lightTexIdx;
        if (idx >= 0 && idx < static_cast<Int32>(_textures.size()))
        {
            if (const Ref<ImageTexture>& pTex = _textures[idx])
            {
                mat.SetLightTexture(pTex);
            }
        }
    }

    if (_importedMat.vertexDisplacementTexIdx != k_invalidImportedIdx)
    {
        const Int32 idx = _importedMat.vertexDisplacementTexIdx;
        if (idx >= 0 && idx < static_cast<Int32>(_textures.size()))
        {
            if (const Ref<ImageTexture>& pTex = _textures[idx])
            {
                mat.SetVertexDisplacementTexture(pTex);
            }
        }
    }

    return mat;
}

Optional<ModelNode> ImportModelDialog::ConvertNodeRecursive_(
    const ImportedModelNode& _node)
{
    ModelNode node;
    node.name      = _node.name;
    node.mtx = _node.mtx;
    node.meshIndices.reserve(_node.meshIdxes.size());
    node.materialIndices.reserve(_node.materialIdxes.size());
    node.children.reserve(_node.children.size());

    // Copy mesh and material indices
    for (const Int32 idx: _node.meshIdxes)
        node.meshIndices.push_back(idx);

    for (const Int32 idx: _node.materialIdxes)
        node.materialIndices.push_back(idx);

    // Process children recursively
    for (const ImportedModelNode& child: _node.children)
    {
        if (Optional<ModelNode> optChild = ConvertNodeRecursive_(child))
        {
            node.children.push_back(std::move(optChild.value()));
        }
    }

    return node;
}

Optional<Model> ImportModelDialog::CreateModelFromImportedModelRootNode_(
    const AssimpImporter& _importer,
    const fs::path&       _baseDir) const
{
    // Get imported data
    const ImportedModelNode&             rootNode  = _importer.GetRootNode();
    const Vector<ImportedMesh>&     meshes    = _importer.GetMeshes();
    const Vector<ImportedMaterial>& materials = _importer.GetMaterials();
    const Vector<ImportedTexture>&  textures  = _importer.GetTextures();
    const VertexLayout&                  layout    = m_pEntry->GetVertexLayout();

    // Create output directories
    const fs::path meshOutputDir = _baseDir / "Meshes";
    const fs::path texOutputDir  = _baseDir / "Textures";
    const fs::path matOutputDir  = _baseDir / "Materials";

    try
    {

        fs::create_directories(meshOutputDir);
        fs::create_directories(matOutputDir);
        if (!textures.empty())
        {
            fs::create_directories(texOutputDir);
        }
    }
    catch (const fs::filesystem_error& ec)
    {
        GlobalBuffer::SetLastError("Failed to create output directories: {}", ec.what());
        return k_nullopt;
    }

    ModelDesc desc;

    // Process meshes
    desc.meshPaths.reserve(meshes.size());
    for (const ImportedMesh& mesh: meshes)
    {
        fs::path outputPath = meshOutputDir / mesh.name;
        outputPath.replace_extension(k_jamMeshExt);

        if (fs::exists(outputPath))
        {
            GlobalBuffer::SetLastError("Mesh file already exists: {}", outputPath.native());
            return k_nullopt;
        }

        Optional<Mesh> optMesh = CreateMeshFromImported_(mesh, layout);
        if (!optMesh || !optMesh->Save(outputPath))
        {
            GlobalBuffer::SetLastError("Failed to save mesh: {}", outputPath.native());
            return k_nullopt;
        }

        desc.meshPaths.push_back(std::move(outputPath));
    }

    // Process textures
    Vector<Ref<ImageTexture>> imageTextures;
    imageTextures.reserve(textures.size());

    for (size_t i = 0; i < textures.size(); ++i)
    {
        const ImportedTexture& tex = textures[i];

        const fs::path outputPath = GenerateTexturePath_(texOutputDir, i, tex);
        if (fs::exists(outputPath))
        {
            GlobalBuffer::SetLastError("Texture file already exists: {}", outputPath.native());
            return k_nullopt;
        }

        Ref<ImageTexture> pImageTexture = CreateImageTextureFromImportedTexture_(tex, outputPath);
        if (!pImageTexture)
        {
            imageTextures.push_back(nullptr);   // Keep indices aligned
            Log::Warn("Skipping invalid texture at index {}", i);
            continue;
        }

        if (!pImageTexture->Save(outputPath))
        {
            GlobalBuffer::SetLastError("Failed to save texture: {}", outputPath.native());
            return k_nullopt;
        }

        imageTextures.push_back(std::move(pImageTexture));
    }

    // Process materials
    desc.matPaths.reserve(materials.size());
    for (const ImportedMaterial& importedMat: materials)
    {
        fs::path outputPath = matOutputDir / importedMat.name;
        outputPath.replace_extension(k_jamMaterialExt);

        if (fs::exists(outputPath))
        {
            GlobalBuffer::SetLastError("Material file already exists: {}", outputPath.native());
            return k_nullopt;
        }

        Optional<Material> optMat = CreateMaterialFromImported_(importedMat, imageTextures);
        if (!optMat || !optMat->Save(outputPath))
        {
            GlobalBuffer::SetLastError("Failed to save material: {}", outputPath.native());
            return k_nullopt;
        }

        desc.matPaths.push_back(std::move(outputPath));
    }

    // Convert root node
    if (Optional<ModelNode> optRootNode = ConvertNodeRecursive_(rootNode))
    {
        desc.rootNode = std::move(optRootNode.value());
    }

    Model model;
    model.LoadMemory(desc);
    return model;
}

}   // namespace jam