#include "pch.h"

#include "AssimpImporter.h"

#include "BaseUtilities.h"
#include "MeshFactory.h"
#include "Profile.h"
#include "StringUtilities.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
using namespace jam;
namespace
{

Int32 k_basicImportFlags = aiProcess_JoinIdenticalVertices
                         | aiProcess_Triangulate
                         | aiProcess_ImproveCacheLocality
                         | aiProcess_OptimizeMeshes
                         | aiProcess_RemoveRedundantMaterials
                         | aiProcess_SortByPType
                         | aiProcess_FindDegenerates
                         | aiProcess_FindInvalidData
                         | aiProcess_RemoveComponent
#ifdef JAM_GFX_API_D3D11
                         | aiProcess_MakeLeftHanded
                         | aiProcess_FlipWindingOrder
                         | aiProcess_FlipUVs;
#else
                           0;
#endif

Int32 k_staticMeshImportFlags = k_basicImportFlags
                              | aiProcess_Debone;

Int32 k_basicRemoveComponentFlags = aiComponent_LIGHTS
                                  | aiComponent_CAMERAS
                                  | aiComponent_TEXTURES;

Int32 k_staticMeshRemoveComponentFlags = k_basicRemoveComponentFlags
                                       | aiComponent_BONEWEIGHTS
                                       | aiComponent_ANIMATIONS;

}   // namespace

namespace jam
{

eErrorCode AssimpImporter::Import()
{
    JAM_ASSERT(!m_filePath.empty(), "File path is empty.");
    JAM_ASSERT(IsImportablePath(m_filePath), "File path is not importable. File path: '{}'", m_filepath.native());

    JAM_PROFILE_SCOPE_MS(R"(AssimpImporter::Import "{}")", m_filePath.native());

    // clean up
    m_meshes.clear();
    m_materials.clear();

    Int32            flags = m_bLoadStaticMesh ? k_staticMeshImportFlags : k_basicImportFlags;
    Assimp::Importer importer;

    if (m_bComputeTangent)
        flags |= aiProcess_CalcTangentSpace;

    if (m_normalGenType == eNormalGenType::Flat)
        flags |= aiProcess_GenNormals;

    if (m_normalGenType == eNormalGenType::Smooth)
        flags |= aiProcess_GenSmoothNormals;

    if (m_dropExistingNormal)
        flags |= aiProcess_DropNormals;

    if (m_bLimitBondWeights)
        flags |= aiProcess_LimitBoneWeights;

    if (m_bLoadStaticMesh)
    {
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, k_staticMeshRemoveComponentFlags);
    }
    else
    {
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, k_basicRemoveComponentFlags);
    }

    // 삼각형만 남기고 다른 프리미티브는 제거
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);

    const aiScene* pScene = importer.ReadFile(m_filePath, flags);
    if (!pScene)
    {
        SetErrorString("AssimpImporter::Import() - Failed to load file: {}", importer.GetErrorString());
        return eErrorCode::FileReadFailed;
    }

    // todo
    // bool bLoadBoneWeight = !m_bLoadStaticMesh;
    // bool bLoadAnimation  = !m_bLoadStaticMesh;

    // 메시
    if (pScene->HasMeshes())
    {
        m_meshes.reserve(pScene->mNumMeshes);
        for (UInt32 i = 0; i < pScene->mNumMeshes; ++i)
        {
            aiMesh* pMesh = pScene->mMeshes[i];

            AssimpMesh mesh;
            mesh.name = std::format("{}_{}", pMesh->mName.C_Str(), i);

            if (pMesh->HasPositions())
            {
                mesh.positions.resize(sizeof(float) * 3 * pMesh->mNumVertices);
                std::memcpy(mesh.positions.data(), pMesh->mVertices, sizeof(Vec3) * pMesh->mNumVertices);
            }

            if (pMesh->HasNormals())
            {
                mesh.normals.resize(sizeof(float) * 3 * pMesh->mNumVertices);
                std::memcpy(mesh.normals.data(), pMesh->mNormals, sizeof(Vec3) * pMesh->mNumVertices);
            }

            if (pMesh->HasTangentsAndBitangents())
            {
                mesh.tangents.resize(sizeof(float) * 3 * pMesh->mNumVertices);
                std::memcpy(mesh.tangents.data(), pMesh->mTangents, sizeof(Vec3) * pMesh->mNumVertices);

                mesh.bitangents.resize(sizeof(float) * 3 * pMesh->mNumVertices);
                std::memcpy(mesh.bitangents.data(), pMesh->mBitangents, sizeof(Vec3) * pMesh->mNumVertices);
            }

            std::vector<Vec3>* texCoordTable[4] = { &mesh.texCoords0, &mesh.texCoords1, &mesh.texCoords2, &mesh.texCoords3 };
            for (UInt32 j = 0; j < 4; ++j)
            {
                if (pMesh->HasTextureCoords(j))
                {
                    std::vector<Vec3>* texCoords = texCoordTable[j];
                    texCoords->resize(sizeof(Vec3) * pMesh->mNumVertices);
                    std::memcpy(texCoords->data(), pMesh->mTextureCoords[j], sizeof(Vec3) * pMesh->mNumVertices);
                }
            }

            std::vector<Vec4>* pColors[4] = { &mesh.colors0, &mesh.colors1, &mesh.colors2, &mesh.colors3 };
            for (UInt32 j = 0; j < 4; ++j)
            {
                if (pMesh->HasVertexColors(j))
                {
                    std::vector<Vec4>* colors = pColors[j];
                    colors->resize(sizeof(Vec4) * pMesh->mNumVertices);
                    std::memcpy(colors->data(), pMesh->mColors[j], sizeof(Vec4) * pMesh->mNumVertices);
                }
            }

            mesh.indexFormat  = std::cmp_less_equal(pMesh->mNumVertices, k_uint16Max) ? eIndexFormat::U16 : eIndexFormat::U32;
            Int32 indexStride = GetIndexFormatByte(mesh.indexFormat);

            mesh.indices.resize(pMesh->mNumFaces * 3 * indexStride);
            for (UInt32 j = 0; j < pMesh->mNumFaces; ++j)
            {
                aiFace& face = pMesh->mFaces[j];
                JAM_ASSERT(face.mNumIndices == 3, "AssimpImporter::Import() - Face is not a triangle");

                for (UInt32 k = 0; k < 3; k++)
                {
                    UINT index  = face.mIndices[k];
                    UINT offset = (j * 3 + k) * indexStride;
                    if (mesh.indexFormat == eIndexFormat::U16)
                    {
                        UInt16* pDst = reinterpret_cast<UInt16*>(&mesh.indices[offset]);
                        *pDst        = NarrowCast<UInt16>(index);
                    }
                    else
                    {
                        UInt32* pDst = reinterpret_cast<UInt32*>(&mesh.indices[offset]);
                        *pDst        = index;
                    }
                }
            }

            // bone + animation -> todo!
            m_meshes.emplace_back(std::move(mesh));
        }
    }

    if (pScene->HasMaterials())
    {
        for (UInt32 i = 0; i < pScene->mNumMaterials; ++i)
        {
            aiMaterial*    pMaterial = pScene->mMaterials[i];
            AssimpMaterial material;

            aiString name;
            if (pMaterial->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
            {
                material.name = std::format("{}_{}", name.C_Str(), i);
            }
            else
            {
                material.name = std::format("mat_{}", i);
            }

            aiColor3D color;
            if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
            {
                material.ambientColor = Vec3 { color.r, color.g, color.b };
            }

            if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
            {
                material.diffuseColor = Vec3 { color.r, color.g, color.b };
            }

            if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
            {
                material.specularColor = Vec3 { color.r, color.g, color.b };
            }

            float shininess = 0.f;
            if (pMaterial->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
            {
                material.shininess = shininess;
            }

            // PBR
            if (pMaterial->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
            {
                material.albedoColor = Vec3 { color.r, color.g, color.b };
            }

            float roughness = 1.f;
            if (pMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
            {
                material.roughness = roughness;
            }

            float metallic = 1.f;
            if (pMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
            {
                material.metallic = metallic;
            }

            float opacity = 1.f;
            if (pMaterial->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
            {
                material.opacity = opacity;
            }

            Int32 twoSided = 0;
            if (pMaterial->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS)
            {
                material.bDoubleSide = twoSided != 0;
            }

            if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS)
            {
                material.emissiveColor = Vec3 { color.r, color.g, color.b };
            }

            float emissiveIntensity = 1.f;
            if (pMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity) == AI_SUCCESS)
            {
                material.emissiveIntensity = emissiveIntensity;
            }

            float bumpscaling = 1.f;
            if (pMaterial->Get(AI_MATKEY_BUMPSCALING, bumpscaling) == AI_SUCCESS)
            {
                material.bumpScaling = bumpscaling;
            }

            m_materials.emplace_back(std::move(material));
        }
    }

    return eErrorCode::Ok;
}

void AssimpImporter::Reset()
{
    m_meshes.clear();
    m_materials.clear();

    m_bComputeTangent    = false;
    m_normalGenType      = eNormalGenType::None;
    m_dropExistingNormal = false;
    m_bLimitBondWeights  = false;
    m_bLoadStaticMesh    = false;
}

void AssimpImporter::ComputeTangent(const bool _bEnable)
{
    m_bComputeTangent = _bEnable;
}

void AssimpImporter::ComputeNormal(const eNormalGenType _type)
{
    m_normalGenType = _type;
}

void AssimpImporter::DropExistingNormal(const bool _bEnable)
{
    m_dropExistingNormal = _bEnable;
}

void AssimpImporter::LimitBoneWeight(const bool _bEnable)
{
    m_bLimitBondWeights = _bEnable;
}

void AssimpImporter::LoadOptimizationForStaticMesh(const bool _bEnable)
{
    m_bLoadStaticMesh = _bEnable;
}

void AssimpImporter::SetPath(const fs::path& _filePath)
{
    m_filePath = _filePath;
}

bool AssimpImporter::IsImportablePath(const fs::path& _filePath) const
{
    std::string      extensions;
    Assimp::Importer importer;
    importer.GetExtensionList(extensions);

    std::vector<std::string> extensionList = Split(extensions, ";");
    // maybe "*.obj;*.fbx;*.dae;"s

    fs::path    inputExt      = _filePath.extension();
    std::string lowerInputExt = ToLower(inputExt.string());
    lowerInputExt.insert(0, "*");
    // maybe *.xxx

    for (const std::string& ext: extensionList)
    {
        if (ext == lowerInputExt)
        {
            return true;
        }
    }

    return false;
}

std::span<const AssimpMesh> AssimpImporter::GetMeshes() const
{
    return m_meshes;
}

std::span<const AssimpMaterial> AssimpImporter::GetMaterials() const
{
    return m_materials;
}

std::vector<Mesh> AssimpImporter::CreateMeshes(const VertexLayout& _layout) const
{
    JAM_ASSERT(!m_meshes.empty(), "No mesh to create. Please call Import() first.");
    JAM_ASSERT(_layout.Has(eVertexAttribute::Position), "Vertex layout must have position attribute.");
    JAM_PROFILE_SCOPE_MS(R"(AssimpImporter::CreateMeshes layout="{}")", _layout.ToString());

    std::vector<Mesh> result;
    result.reserve(m_meshes.size());

    for (const AssimpMesh& assimpMesh: m_meshes)
    {
        CreateMeshInput input;

        MemoryView* dst[EnumCount<eVertexAttribute>()] = {
            &input.positionData,          // Position
            &input.normalDataOrNull,      // Normal
            &input.tangentDataOrNull,     // Tangent
            &input.bitangentDataOrNull,   // Bitangent
            &input.color0DataOrNull,      // Color0
            &input.color1DataOrNull,      // Color1
            &input.color2DataOrNull,      // Color2
            &input.color3DataOrNull,      // Color3
            &input.texcoord0DataOrNull,   // TexCoord0
            &input.texcoord1DataOrNull,   // TexCoord1
            &input.texcoord2DataOrNull,   // TexCoord2
            &input.texcoord3DataOrNull,   // TexCoord3
        };

        MemoryView src[EnumCount<eVertexAttribute>()] = {
            MemoryView { assimpMesh.positions },    // Position
            MemoryView { assimpMesh.normals },      // Normal
            MemoryView { assimpMesh.tangents },     // Tangent
            MemoryView { assimpMesh.bitangents },   // Bitangent
            MemoryView { assimpMesh.colors0 },      // Color0
            MemoryView { assimpMesh.colors1 },      // Color1
            MemoryView { assimpMesh.colors2 },      // Color2
            MemoryView { assimpMesh.colors3 },      // Color3
            MemoryView { assimpMesh.texCoords0 },   // TexCoord0
            MemoryView { assimpMesh.texCoords1 },   // TexCoord1
            MemoryView { assimpMesh.texCoords2 },   // TexCoord2
            MemoryView { assimpMesh.texCoords3 }    // TexCoord3
        };

        for (eVertexAttribute attrib: EnumRange<eVertexAttribute>())
        {
            MemoryView& srcData = src[EnumToInt(attrib)];
            if (_layout.Has(attrib) && srcData.IsValid())
            {
                *dst[EnumToInt(attrib)] = srcData;
            }
        }

        input.indexData   = MemoryView { assimpMesh.indices };
        input.indexFormat = assimpMesh.indexFormat;

        result.emplace_back(CreateMesh(input, _layout));
    }

    return result;
}

std::vector<Material> AssimpImporter::CreateMaterial() const
{
    JAM_ASSERT(!m_materials.empty(), "No material to create. Please call Import() first.");
    JAM_PROFILE_SCOPE_MS(R"(AssimpImporter::CreateMaterial count={})", m_materials.size());

    std::vector<Material> result;
    result.reserve(m_materials.size());

    for (const AssimpMaterial& assimpMat: m_materials)
    {
        MaterialParams mat;
        mat.ambientColor  = assimpMat.ambientColor;
        mat.diffuseColor  = assimpMat.diffuseColor;
        mat.specularColor = assimpMat.specularColor;
        mat.shininess     = assimpMat.shininess;

        mat.albedoColor = assimpMat.albedoColor;
        mat.roughness   = assimpMat.roughness;
        mat.metallic    = assimpMat.metallic;

        mat.emissiveColor     = assimpMat.emissiveColor;
        mat.emissiveIntensity = assimpMat.emissiveIntensity;

        mat.opacity        = assimpMat.opacity;
        mat.bIsTransparent = assimpMat.opacity < 1.0f;

        mat.bDoubleSide    = assimpMat.bDoubleSide;
        mat.bumpMapScaling = assimpMat.bumpScaling;

        Material material;
        material.SetParams(mat);
        result.emplace_back(std::move(material));
    }

    return result;
}

}   // namespace jam