#pragma once

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

constexpr Int32 k_invalidImportedIdx = -1;

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

struct ImportedMaterialTextures
{
    ImportedMaterialTextures()
    {
        idxes.fill(k_invalidImportedIdx);
    }

    void Set(const eMaterialTextureType _type, const Int32 _idx)
    {
        idxes[UnderlyingFrom(_type)] = _idx;
    }

    [[nodiscard]] Int32 Get(const eMaterialTextureType _type) const
    {
        return idxes[UnderlyingFrom(_type)];
    }

    [[nodiscard]] bool Has(const eMaterialTextureType _type) const
    {
        return Get(_type) != k_invalidImportedIdx;
    }

    Array<Int32, CountOf<eMaterialTextureType>()> idxes = {};
};

struct ImportedMaterial
{
    String                   name     = {};
    MaterialParams           params   = {};
    MaterialTextureSettings  settings = {};
    ImportedMaterialTextures texes    = {};
};

struct ImportedTexture
{
    String       name          = {};
    CMemoryBlock pixels        = {};
    Int32        width         = 0;
    Int32        height        = 0;
    eImageFormat format        = {};
    bool         bIsCompressed = false;
    bool         bPlaceholder  = false;   // for import failed
};

struct AssimpImporterOptions
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
    [[nodiscard]] static Expect<AssimpImporter> ImportFromFile(const Path& _path, const AssimpImporterOptions& _options = {});

    [[nodiscard]] const ImportedModelNode&        GetRootNode() const;
    [[nodiscard]] const Vector<ImportedMesh>&     GetMeshes() const;
    [[nodiscard]] const Vector<ImportedMaterial>& GetMaterials() const;
    [[nodiscard]] const Vector<ImportedTexture>&  GetTextures() const;

    [[nodiscard]] Expect<Ref<Model>> ConvertToModel(const VertexLayout& _vl) const;

private:
    AssimpImporter() = default;
    Error ImportFromFile_(const Path& _path, const AssimpImporterOptions& _options);

    [[nodiscard]] Vector<Ref<Mesh>>            CreateMeshes_(const VertexLayout& _vl) const;
    [[nodiscard]] Expect<Vector<Ref<Texture>>> CreateTextures_() const;
    [[nodiscard]] Vector<Ref<Material>>        CreateMaterials_(const Vector<Ref<Texture>>& _texes) const;

    void  ImportMeshes_(const aiScene* _pScene, const AssimpImporterOptions& _options);
    Error ImportTextures_(const aiScene* _pScene);
    void  ImportMaterials_(const aiScene* _pScene);

    void                            ImportModelNode_(const aiScene* _pScene);
    [[nodiscard]] ImportedModelNode CreateImportModelNode_Recursive_(const aiScene* _pScene, const aiNode* _pNode);

    ImportedModelNode        m_root      = {};
    Vector<ImportedMesh>     m_meshes    = {};
    Vector<ImportedMaterial> m_materials = {};
    Vector<ImportedTexture>  m_textures  = {};
};

}   // namespace Jam