#pragma once
#include <assimp/Importer.hpp>

#include "EnumArray.h"
#include "Material.h"
#include "Model.h"

struct aiNode;
struct aiTexture;
struct aiMaterial;
struct aiScene;
struct aiMesh;

namespace Jam
{

enum class eImageFormat;
struct ModelNode;
class Texture;
class Material;
class Model;
class Mesh;

constexpr Int32 k_invalidImportedTextureIdx = -1;

struct ImportedMesh
{
    String name = {};

    Vector<Vec3>           positions  = {};
    Vector<Vec3>           normals    = {};
    Vector<Vec3>           tangents   = {};
    Vector<Vec3>           bitangents = {};
    Array<Vector<Vec4>, 4> colors     = {};
    Array<Vector<Vec2>, 4> texCoords  = {};
    Vector<Byte>           idxData    = {};
    bool                   bUseU32    = false;
};

using ImportedMaterialTexture = EnumArray<eMaterialTextureType, Int32>;

struct ImportedMaterial
{
    ImportedMaterial()
    {
        textures.Fill(k_invalidImportedTextureIdx);
    }

    String                  name     = {};
    MaterialParams          params   = {};
    MaterialTextureSettings settings = {};
    ImportedMaterialTexture textures = {};
};

struct ImportedTexture
{
    String       name          = {};
    MemoryView      pixels        = {};
    Int32        width         = 0;
    Int32        height        = 0;
    eImageFormat format        = {};
    bool         bIsCompressed = false;
    bool         bPlaceholder  = false;   // for import failed
};

struct AssimpImporterInfo
{
    bool bGenerateNormals          = false;
    bool bDropExistingNormals      = false;
    bool bGenerateTangentFrame     = false;
    bool bDropExistingTangentFrame = false;
    bool bLimitBondWeights         = false;
    bool bLoadStaticMesh           = false;
};

using ImportedModelNode = ModelNode;   // 동일한 구조이므로 별도 정의 없이 재사용

class AssimpImporter
{
public:
    [[nodiscard]] static Expect<AssimpImporter> ImportFromMemory(MemoryView _data, const AssimpImporterInfo& _info = {});
    [[nodiscard]] static Expect<AssimpImporter> ImportFromFile(StringView _path, const AssimpImporterInfo& _info = {});

    [[nodiscard]] const ImportedModelNode&        GetRootNode() const;
    [[nodiscard]] const Vector<ImportedMesh>&     GetMeshes() const;
    [[nodiscard]] const Vector<ImportedMaterial>& GetMaterials() const;
    [[nodiscard]] const Vector<ImportedTexture>&  GetTextures() const;
    [[nodiscard]] Expect<Ref<Model>>              ExtractToModel(const VertexLayout& _vl);   // AssimpImporter is invalidate after successful extraction

private:
    AssimpImporter() = default;
    Error  ImportFromMemory_(MemoryView _data, const AssimpImporterInfo& _options);
    Error  ImportFromFile_(StringView _path, const AssimpImporterInfo& _options);
    Error  Import_(const aiScene* _pScene, const AssimpImporterInfo& _options);
    UInt32 MakeImportFlags_(const AssimpImporterInfo& _options) const;
    void   PreImport_(Assimp::Importer& _importer, const AssimpImporterInfo& _options) const;

    // import steps
    void                            ImportMeshes_(const aiScene* _pScene, const AssimpImporterInfo& _options);
    Error                           ImportTextures_(const aiScene* _pScene);
    void                            ImportMaterials_(const aiScene* _pScene);
    void                            ImportModelNode_(const aiScene* _pScene);
    [[nodiscard]] ImportedModelNode ImportModelNode_Recursive_(const aiScene* _pScene, const aiNode* _pNode);

    // convert
    [[nodiscard]] Vector<Ref<Mesh>>            ConvertToMeshes_(const VertexLayout& _vl) const;
    [[nodiscard]] Expect<Vector<Ref<Texture>>> ConvertToTextures_() const;
    [[nodiscard]] Vector<Ref<Material>>        ConvertToMaterials_(const Vector<Ref<Texture>>& _textures) const;

    bool                     m_bExtracted        = false;
    ImportedModelNode        m_importedRoot      = {};
    Vector<ImportedMesh>     m_importedMeshes    = {};
    Vector<ImportedMaterial> m_importedMaterials = {};
    Vector<ImportedTexture>  m_importedTextures  = {};
};

}   // namespace Jam