#pragma once
#include "IImporter.h"
#include "ImageBase.h"
#include "Material.h"

#include <assimp/mesh.h>
#include <assimp/scene.h>

namespace jam
{

constexpr Int32 k_invalidImportedTexIndex = -1;

struct ImportedMesh
{
    // bone weight, bone index 추후 구현 예정

    std::string name;

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
    std::string name;

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

    Int32 albedoTexIndex             = k_invalidImportedTexIndex;
    Int32 normalTexIndex             = k_invalidImportedTexIndex;
    Int32 metallicTexIndex           = k_invalidImportedTexIndex;
    Int32 roughnessTexIndex          = k_invalidImportedTexIndex;
    Int32 aoTexIndex                 = k_invalidImportedTexIndex;
    Int32 emissiveTexIndex           = k_invalidImportedTexIndex;
    Int32 lightTexIndex              = k_invalidImportedTexIndex;
    Int32 vertexDisplacementTexIndex = k_invalidImportedTexIndex;
};

struct ImportedTexture
{
    // only embedded texture is supported
    ConstMemView pixels        = {};
    Int32        width         = 0;
    Int32        height        = 0;
    eImageFormat format        = {};
    bool         bIsCompressed = false;
};

struct ImportedModelNode
{
    std::string   name;
    Mat4          mtx = Mat4::Identity;
    Vector<Int32> meshIndices;
    Vector<Int32> materialIndices;

    Vector<ImportedModelNode> children;
};

class AssimpImporter : public IImporter
{
public:
    bool Import() override;

    void SetPath(const fs::path& _path) override;
    void GenerateTangentFrame();
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

    fs::path m_path                  = {};
    bool     m_bGenerateTangentFrame = false;
    bool     m_bGenerateNormals      = false;
    bool     m_bDropExistingNormals  = false;
    bool     m_bLimitBondWeights     = false;
    bool     m_bLoadStaticMesh       = false;
};

}   // namespace jam