#pragma once

#include "IImporter.h"
#include "ImageBase.h"
#include "Material.h"

struct aiMaterial;
struct aiScene;

namespace jam
{

constexpr Int32 k_invalidImportedIdx = -1;

struct ImportedMesh
{
    // bone weight, bone index 추후 구현 예정

    std::string name = {};

    Vector<Vec3> positions    = {};
    Vector<Vec3> normals      = {};
    Vector<Vec3> tangents     = {};
    Vector<Vec3> bitangents   = {};
    Vector<Vec4> colors[4]    = {};
    Vector<Vec2> texCoords[4] = {};
    Vector<Byte> idxData      = {};
    bool         bUseU32      = false;
};

struct ImportedMaterial
{
    std::string name = {};

    Vec3  ambientColor  = Vec3::One;
    Vec3  diffuseColor  = Vec3::One;
    Vec3  specularColor = Vec3::One;
    float shininess     = 0.f;

    Vec3  albedoColor  = Vec3::One;
    float roughness    = 1.f;
    float metallic     = 1.f;
    bool  bDoubleSided = false;

    Vec3  emissiveColor     = Vec3::Zero;
    float emissiveIntensity = 1.f;

    eTextureChannel metallicTexChannel  = eTextureChannel::Red;
    eTextureChannel roughnessTexChannel = eTextureChannel::Red;

    Int32 albedoTexIdx             = k_invalidImportedIdx;
    Int32 normalTexIdx             = k_invalidImportedIdx;
    Int32 metallicTexIdx           = k_invalidImportedIdx;
    Int32 roughnessTexIdx          = k_invalidImportedIdx;
    Int32 aoTexIdx                 = k_invalidImportedIdx;
    Int32 emissiveTexIdx           = k_invalidImportedIdx;
    Int32 lightTexIdx              = k_invalidImportedIdx;
    Int32 vertexDisplacementTexIdx = k_invalidImportedIdx;
};

struct ImportedTexture
{
    // only embedded texture is supported
    CMemView     pixels        = {};
    Int32        width         = 0;
    Int32        height        = 0;
    eImageFormat format        = {};
    bool         bIsCompressed = false;
    bool         bTombstone    = false;   // for import failed
};

struct ImportedModelNode
{
    std::string   name          = {};
    Mat4          mtx           = Mat4::Identity;
    Vector<Int32> meshIdxes     = {};
    Vector<Int32> materialIdxes = {};

    Vector<ImportedModelNode> children = {};
};

class AssimpImporter : public IImporter
{
public:
    bool Import() override;

    void SetPath(const fs::path& _path) override;
    void GenerateTangentFrame(bool _bDropExistTangents);
    void GenerateNormals(bool _bDropExistNormals);
    void LimitBoneWeight();
    void LoadOptimizationForStaticMesh();

    [[nodiscard]] const ImportedModelNode&        GetRootNode() const;
    [[nodiscard]] const Vector<ImportedMesh>&     GetMeshes() const;
    [[nodiscard]] const Vector<ImportedMaterial>& GetMaterials() const;
    [[nodiscard]] const Vector<ImportedTexture>&  GetTextures() const;

private:
    [[nodiscard]] Vector<ImportedMesh>     ImportMeshesFrom_(const aiScene* _pScene) const;
    [[nodiscard]] Vector<ImportedTexture>  ImportTexturesFrom_(const aiScene* _pScene) const;
    [[nodiscard]] Vector<ImportedMaterial> ImportMaterialsFrom_(const aiScene* _pScene) const;

    ImportedModelNode        m_rootNode  = {};
    Vector<ImportedMesh>     m_meshes    = {};
    Vector<ImportedMaterial> m_materials = {};
    Vector<ImportedTexture>  m_textures  = {};

    fs::path m_path                      = {};
    bool     m_bGenerateTangentFrame     = false;
    bool     m_bDropExistingTangentFrame = false;
    bool     m_bGenerateNormals          = false;
    bool     m_bDropExistingNormals      = false;
    bool     m_bLimitBondWeights         = false;
    bool     m_bLoadStaticMesh           = false;
};

}   // namespace jam