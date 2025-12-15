#pragma once
#include "AssimpImporter.h"
#include "Model.h"
#include "PopupDialog.h"
#include "VertexLayoutStorage.h"

namespace jam
{

class ImportModelDialog
{
public:
    explicit ImportModelDialog(const fs::path& _path);
    void operator()();

private:
    bool                               Widget_NormalGenSelector_(eNormalGenType& _type) const;
    bool                               OnClickImportButton_();
    [[nodiscard]] std::optional<Model> CreateModelFromImportedModelRootNode_(const AssimpImporter& _importer, const fs::path& _baseDir) const;

    // Helper functions for mesh processing
    static void CopyVec3Attribute_(
        Vector<Byte>&       _vertexData,
        const Vector<Vec3>& _srcData,
        Int32                    _numVerts,
        Int32                    _vertexStride,
        Int32                    _offset,
        Int32                    _byteWidth);

    static void CopyVec4Attribute_(
        Vector<Byte>&       _vertexData,
        const Vector<Vec4>& _srcData,
        Int32                    _numVerts,
        Int32                    _vertexStride,
        Int32                    _offset,
        Int32                    _byteWidth);

    [[nodiscard]] static std::optional<Mesh> CreateMeshFromImported_(
        const ImportedMesh& _mesh,
        const VertexLayout& _layout);

    // Helper functions for texture processing
    [[nodiscard]] static Ref<ImageTexture> CreateImageTextureFromImportedTexture_(
        const ImportedTexture& _tex,
        const fs::path&        _virtualPath);

    [[nodiscard]] static fs::path GenerateTexturePath_(
        const fs::path&        _outputDir,
        size_t                 _index,
        const ImportedTexture& _tex);

    // Helper functions for material processing
    [[nodiscard]] static std::optional<Material> CreateMaterialFromImported_(
        const ImportedMaterial&               _importedMat,
        const Vector<Ref<ImageTexture>>& _textures);

    // Helper functions for node processing
    [[nodiscard]] static std::optional<ModelNode> ConvertNodeRecursive_(
        const ImportedModelNode& _node);

    PopupDialog            m_popup;
    fs::path               m_parentDir;
    std::string            m_buf;
    Ref<VertexLayoutEntry> m_pEntry;

    fs::path       m_importPath;
    bool           m_bComputeTangent  = false;
    eNormalGenType m_normalGenType    = eNormalGenType::None;
    bool           m_bDropNormal      = false;
    bool           m_bLimitBoneWeight = false;
    bool           m_bStaticMesh      = false;
};

}   // namespace jam