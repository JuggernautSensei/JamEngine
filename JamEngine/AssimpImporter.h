#pragma once
#include "IImporter.h"
#include "ImageBase.h"
#include "Material.h"
#include "Memory.h"
#include "Mesh.h"

#include <assimp/Importer.hpp>

namespace jam
{

enum class eNormalGenType
{
    None,
    Flat,
    Smooth
};

constexpr Int32 k_invalidImportedTextureIndex = -1;

struct ImportedMesh
{
    // bone weight, bone index 추후 구현 예정

    std::string name;

    std::vector<Vec3> positions  = {};
    std::vector<Vec3> normals    = {};
    std::vector<Vec3> tangents   = {};
    std::vector<Vec3> bitangents = {};
    std::vector<Vec4> colors0    = {};
    std::vector<Vec4> colors1    = {};
    std::vector<Vec4> colors2    = {};
    std::vector<Vec4> colors3    = {};
    std::vector<Vec3> texCoords0 = {};
    std::vector<Vec3> texCoords1 = {};
    std::vector<Vec3> texCoords2 = {};
    std::vector<Vec3> texCoords3 = {};

    std::vector<Byte> indices = {};
    bool              bUseU32 = false;
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

    Int32 albedoTexIndex             = k_invalidImportedTextureIndex;
    Int32 normalTexIndex             = k_invalidImportedTextureIndex;
    Int32 metallicTexIndex           = k_invalidImportedTextureIndex;
    Int32 roughnessTexIndex          = k_invalidImportedTextureIndex;
    Int32 aoTexIndex                 = k_invalidImportedTextureIndex;
    Int32 emissiveTexIndex           = k_invalidImportedTextureIndex;
    Int32 lightTexIndex              = k_invalidImportedTextureIndex;
    Int32 vertexDisplacementTexIndex = k_invalidImportedTextureIndex;
};

struct ImportedTexture
{
    // only embedded texture is supported
    ConstMemView pixels        = {};
    Int32        width         = 0;
    Int32        height        = 0;
    eImageFormat format        = eImageFormat::Unknown;
    bool         bIsCompressed = false;
};

struct ImportedModelNode
{
    std::string        name;
    Mat4               localTransform = Mat4::Identity;
    std::vector<Int32> meshIndices;
    std::vector<Int32> materialIndices;

    std::vector<ImportedModelNode> children;
};

class AssimpImporter : public IImporter
{
public:
    bool Import() override;

    void SetPath(const fs::path& _path) override;
    void ComputeTangent();
    void ComputeNormal(eNormalGenType _type);
    void DropExistingNormal();
    void LimitBoneWeight();
    void LoadOptimizationForStaticMesh();

    [[nodiscard]] const ImportedModelNode&             GetRootNode() const;
    [[nodiscard]] const std::vector<ImportedMesh>&     GetMeshes() const;
    [[nodiscard]] const std::vector<ImportedMaterial>& GetMaterials() const;
    [[nodiscard]] const std::vector<ImportedTexture>&  GetTextures() const;

private:
    Assimp::Importer m_importer;

    ImportedModelNode             m_rootNode  = {};
    std::vector<ImportedMesh>     m_meshes    = {};
    std::vector<ImportedMaterial> m_materials = {};
    std::vector<ImportedTexture>  m_textures  = {};

    fs::path       m_path               = {};
    bool           m_bComputeTangent    = false;
    eNormalGenType m_normalGenType      = eNormalGenType::None;
    bool           m_dropExistingNormal = false;
    bool           m_bLimitBondWeights  = false;
    bool           m_bLoadStaticMesh    = false;
};

}   // namespace jam