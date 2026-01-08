#pragma once

#include <assimp/mesh.h>
#include <assimp/texture.h>

#include "ImageBase.h"
#include "Material.h"
#include "Model.h"

struct aiMaterial;
struct aiScene;

namespace jam
{

struct ModelNode;
class Mesh;
class Material;
class Model;
class ImageTexture;

constexpr Int32 k_invalidImportedIdx = -1;

struct ImportedMesh
{
    std::string name = {};

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
    void Set(const eMaterialTextureType _type, const Int32 _idx)
    {
        mapIdxes[IntFrom(_type)] = _idx;
    }

    Int32 Get(const eMaterialTextureType _type) const
    {
        return mapIdxes[IntFrom(_type)];
    }

    Array<Int32, NumEnums<eMaterialTextureType>()> mapIdxes = {};
};

struct ImportedMaterial
{
    std::string name = {};

    MaterialParams           params   = {};
    MaterialTextureSettings  settings = {};
    ImportedMaterialTextures textures = {};
};

struct ImportedTexture
{
    CMemView     pixels        = {};
    Int32        width         = 0;
    Int32        height        = 0;
    eImageFormat format        = {};
    bool         bIsCompressed = false;
    bool         bPlaceholder  = false;   // for import failed
};

struct AssimpImportOptions
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
    bool Import(const fs::path& _path);

    void GenerateTangentFrame(bool _bDropExistTangents);
    void GenerateNormals(bool _bDropExistNormals);
    void LimitBoneWeight();
    void LoadOptimizationForStaticMesh();
    void SetOptions(const AssimpImportOptions& _options);

    [[nodiscard]] const ImportedModelNode&        GetRootNode() const;
    [[nodiscard]] const Vector<ImportedMesh>&     GetMeshes() const;
    [[nodiscard]] const Vector<ImportedMaterial>& GetMaterials() const;
    [[nodiscard]] const Vector<ImportedTexture>&  GetTextures() const;

    [[nodiscard]] Ref<Model> CreateModel(const VertexLayout& _vl) const;

private:
    void CleanUp_();

    [[nodiscard]] Vector<Ref<Mesh>>         CreateMeshes_(const VertexLayout& _vl) const;
    [[nodiscard]] Vector<Ref<ImageTexture>> CreateTextures_() const;
    [[nodiscard]] Vector<Ref<Material>>     CreateMaterials_(const Vector<Ref<ImageTexture>>& _textures) const;

    [[nodiscard]] Vector<ImportedMesh>     CreateImportedMeshes_(aiMesh** _ppMesh, Int32 _numMeshes) const;
    [[nodiscard]] Vector<ImportedTexture>  CreateImportedTextures_(aiTexture** _pTexture, Int32 _numTextures) const;
    [[nodiscard]] Vector<ImportedMaterial> CreateImportedMaterials_(aiMaterial** _pMats, Int32 _numMats) const;

    ImportedModelNode        m_rootNode  = {};
    Vector<ImportedMesh>     m_meshes    = {};
    Vector<ImportedMaterial> m_materials = {};
    Vector<ImportedTexture>  m_textures  = {};

    AssimpImportOptions m_options = {};
};

}   // namespace jam