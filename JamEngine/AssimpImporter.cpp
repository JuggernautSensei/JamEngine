#include "pch.h"

#include "AssimpImporter.h"

#include "BaseUtilities.h"
#include "MeshFactory.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace jam
{

[[nodiscard]] static Int32 ParseMaterialTextureIndex(const aiMaterial* _pMat, const aiTextureType _type, const Int32 _boundaryIndex)
{
    aiString str;
    if (_pMat->GetTexture(_type, 0, &str) == AI_SUCCESS)
    {
        const char* c = str.C_Str() + 1;   // skip '*'
        try
        {
            const Int32 index = std::stoi(c);
            return (index >= 0 && index < _boundaryIndex) ? index : -1;
        }
        catch (...)
        {
            return -1;
        }
    }
    return -1;
}

[[nodiscard]] static ImportedModelNode BuildImportedModelTree_Recursive(const aiNode* _pNode, const aiScene* _pScene)
{
    ImportedModelNode node;
    if (_pNode->mName.length > 0)
    {
        node.name = _pNode->mName.C_Str();
    }
    else
    {
        node.name = "";
    }

    const aiMatrix4x4& m = _pNode->mTransformation;

    Mat4 mat = {};
    mat._11  = m.a1;
    mat._12  = m.a2;
    mat._13  = m.a3;
    mat._14  = m.a4;
    mat._21  = m.b1;
    mat._22  = m.b2;
    mat._23  = m.b3;
    mat._24  = m.b4;
    mat._31  = m.c1;
    mat._32  = m.c2;
    mat._33  = m.c3;
    mat._34  = m.c4;
    mat._41  = m.d1;
    mat._42  = m.d2;
    mat._43  = m.d3;
    mat._44  = m.d4;

    node.localTransform = mat;

    if (_pNode->mNumMeshes > 0)
    {
        for (UINT i = 0; i < _pNode->mNumMeshes; ++i)
        {
            const UINT meshIndex = _pNode->mMeshes[i];
            node.meshIndices.emplace_back(meshIndex);
            const aiMesh* pMesh = _pScene->mMeshes[meshIndex];
            node.materialIndices.emplace_back(pMesh->mMaterialIndex);
        }
    }

    // children
    UINT numChildren = _pNode->mNumChildren;
    node.children.reserve(numChildren);
    for (UINT i = 0; i < numChildren; ++i)
    {
        node.children.emplace_back(BuildImportedModelTree_Recursive(_pNode->mChildren[i], _pScene));
    }

    return node;
}

}   // namespace jam

namespace jam
{

bool AssimpImporter::Import()
{
    m_rootNode = {};
    m_meshes.clear();
    m_materials.clear();
    m_textures.clear();

    UINT32 flags = aiProcess_JoinIdenticalVertices
                 | aiProcess_Triangulate
                 | aiProcess_ImproveCacheLocality
                 | aiProcess_OptimizeMeshes
                 | aiProcess_RemoveRedundantMaterials
                 | aiProcess_SortByPType
                 | aiProcess_FindDegenerates
                 | aiProcess_FindInvalidData
                 | aiProcess_RemoveComponent;

#ifdef JAM_GFX_API_D3D11
    flags |= aiProcess_MakeLeftHanded
           | aiProcess_FlipWindingOrder
           | aiProcess_FlipUVs;
#endif

    if (m_bLoadStaticMesh)
    {
        flags |= aiProcess_Debone;
    }

    if (m_bComputeTangent)
    {
        flags |= aiProcess_CalcTangentSpace;
    }

    if (m_normalGenType == eNormalGenType::Flat)
    {
        flags |= aiProcess_GenNormals;
    }

    if (m_normalGenType == eNormalGenType::Smooth)
    {
        flags |= aiProcess_GenSmoothNormals;
    }

    if (m_dropExistingNormal)
    {
        flags |= aiProcess_DropNormals;
    }

    if (m_bLimitBondWeights)
    {
        flags |= aiProcess_LimitBoneWeights;
    }

    if (m_bLoadStaticMesh)
    {
        flags |= aiProcess_Debone;
    }

    if (m_bLoadStaticMesh)
    {
        m_importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS);
    }
    else
    {
        m_importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS | aiComponent_BONEWEIGHTS | aiComponent_ANIMATIONS);
    }

    // remove line & point primitives
    m_importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);

    // scene load
    const aiScene* pScene = m_importer.ReadFile(m_path.native(), flags);
    if (!pScene)
    {
        SetErrorString("failed to load file: {}", m_importer.GetErrorString());
        return false;
    }

    // mesh load
    if (pScene->HasMeshes())
    {
        aiMesh* const* pMeshes = pScene->mMeshes;
        const UINT     numMesh = pScene->mNumMeshes;

        m_meshes.reserve(numMesh);

        for (UINT i = 0; i < numMesh; ++i)
        {
            const aiMesh* pMesh = pMeshes[i];
            ImportedMesh  mesh;

            // name
            if (pMesh->mName.length > 0)
            {
                mesh.name = std::format("{}_{}", pMesh->mName.C_Str(), i);
            }
            else
            {
                mesh.name = std::format("mesh_{}", i);
            }

            const UInt32 numVerts = pMesh->mNumVertices;

            // positions
            if (pMesh->HasPositions())
            {
                mesh.positions.resize(numVerts);
                for (UInt32 v = 0; v < numVerts; ++v)
                {
                    const aiVector3D& src = pMesh->mVertices[v];
                    mesh.positions[v]     = Vec3 { src.x, src.y, src.z };
                }
            }

            // normals
            if (pMesh->HasNormals())
            {
                mesh.normals.resize(numVerts);
                for (UInt32 v = 0; v < numVerts; ++v)
                {
                    const aiVector3D& src = pMesh->mNormals[v];
                    mesh.normals[v]       = Vec3 { src.x, src.y, src.z };
                }
            }

            // tangents & bitangents
            if (pMesh->HasTangentsAndBitangents())
            {
                mesh.tangents.resize(numVerts);
                mesh.bitangents.resize(numVerts);
                for (UInt32 v = 0; v < numVerts; ++v)
                {
                    const aiVector3D& t = pMesh->mTangents[v];
                    const aiVector3D& b = pMesh->mBitangents[v];
                    mesh.tangents[v]    = Vec3 { t.x, t.y, t.z };
                    mesh.bitangents[v]  = Vec3 { b.x, b.y, b.z };
                }
            }

            // texture coordinates (up to 4)
            for (UInt32 tc = 0; tc < 4; ++tc)
            {
                if (pMesh->HasTextureCoords(tc))
                {
                    std::vector<Vec3>& dst = (tc == 0) ? mesh.texCoords0
                                           : (tc == 1) ? mesh.texCoords1
                                           : (tc == 2) ? mesh.texCoords2
                                                       : mesh.texCoords3;

                    dst.resize(numVerts);
                    for (UInt32 v = 0; v < numVerts; ++v)
                    {
                        const aiVector3D& src = pMesh->mTextureCoords[tc][v];
                        dst[v]                = Vec3 { src.x, src.y, src.z };
                    }
                }
            }

            // vertex colors (up to 4)
            for (UInt32 cc = 0; cc < 4; ++cc)
            {
                if (pMesh->HasVertexColors(cc))
                {
                    std::vector<Vec4>& dst = (cc == 0) ? mesh.colors0
                                           : (cc == 1) ? mesh.colors1
                                           : (cc == 2) ? mesh.colors2
                                                       : mesh.colors3;

                    dst.resize(numVerts);
                    for (UInt32 v = 0; v < numVerts; ++v)
                    {
                        const aiColor4D& src = pMesh->mColors[cc][v];
                        dst[v]               = Vec4 { src.r, src.g, src.b, src.a };
                    }
                }
            }

            // indices
            {
                mesh.indexFormat         = NeedU32Index(numVerts);
                const Int32  indexStride = GetIndexFormatByte(mesh.indexFormat);
                const UInt32 numFaces    = pMesh->mNumFaces;
                const UInt32 numIndices  = numFaces * 3;

                mesh.indices.resize(numIndices * indexStride);

                for (UInt32 f = 0; f < numFaces; ++f)
                {
                    const aiFace& face = pMesh->mFaces[f];
                    JAM_ASSERT(face.mNumIndices == 3, "AssimpImporter::Import() - Face is not a triangle");

                    for (UInt32 k = 0; k < 3; ++k)
                    {
                        const UInt32 dstOffset = f * 3 + k;
                        const UInt32 index     = face.mIndices[k];

                        if (mesh.indexFormat == eIndexFormat::U16)
                        {
                            UInt16* pDst = reinterpret_cast<UInt16*>(mesh.indices.data() + indexStride * dstOffset);
                            *pDst        = NarrowCast<UInt16>(index);
                        }
                        else
                        {
                            UInt32* pDst = reinterpret_cast<UInt32*>(mesh.indices.data() + indexStride * dstOffset);
                            *pDst        = index;
                        }
                    }
                }
            }

            m_meshes.emplace_back(std::move(mesh));
        }
    }

    // texture load
    const bool bTextureEmbeded = (pScene->mNumTextures > 0);
    if (bTextureEmbeded)
    {
        aiTexture* const* pTexes   = pScene->mTextures;
        const UINT        numTexes = pScene->mNumTextures;
        m_textures.reserve(numTexes);
        for (UINT i = 0; i < numTexes; ++i)
        {
            const aiTexture* pTex          = pTexes[i];
            const bool       bIsCompressed = (pTex->mHeight == 0);

            ImportedTexture texture;
            texture.width         = static_cast<Int32>(pTex->mWidth);
            texture.height        = static_cast<Int32>(pTex->mHeight);
            texture.bIsCompressed = bIsCompressed;
            if (bIsCompressed)
            {
                // Compressed texture
                const size_t           byteWidth  = texture.width;
                const Byte*            pPixel     = reinterpret_cast<const Byte*>(pTex->pcData);
                const std::string_view formatHint = { pTex->achFormatHint, strlen(pTex->achFormatHint) };
                texture.pixels                    = ConstMemView { pPixel, byteWidth };
                texture.format                    = ImageFormatFromString(formatHint);
            }
            else
            {
                // BGRA 로 가정
                const size_t byteWidth = texture.width * texture.height * sizeof(aiTexel);
                const Byte*  pPixel    = reinterpret_cast<const Byte*>(pTex->pcData);
                texture.pixels         = ConstMemView { pPixel, byteWidth };
            }
            m_textures.emplace_back(std::move(texture));
        }
    }

    // material load
    if (pScene->HasMaterials())
    {
        aiMaterial* const* pMats   = pScene->mMaterials;
        const UINT         numMats = pScene->mNumMaterials;

        m_materials.reserve(numMats);
        for (UINT i = 0; i < numMats; ++i)
        {
            const aiMaterial* pMat = pMats[i];
            ImportedMaterial  material;

            // Name
            aiString name;
            if (pMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS && name.length > 0)
            {
                material.name = std::format("{}_{}", name.C_Str(), i);
            }
            else
            {
                material.name = std::format("mat_{}", i);
            }

            // Ambient
            aiColor3D color3;
            if (pMat->Get(AI_MATKEY_COLOR_AMBIENT, color3) == AI_SUCCESS)
            {
                material.ambientColor = Vec3 { color3.r, color3.g, color3.b };
            }

            // Diffuse
            if (pMat->Get(AI_MATKEY_COLOR_DIFFUSE, color3) == AI_SUCCESS)
            {
                material.diffuseColor = Vec3 { color3.r, color3.g, color3.b };
            }

            // Specular
            if (pMat->Get(AI_MATKEY_COLOR_SPECULAR, color3) == AI_SUCCESS)
            {
                material.specularColor = Vec3 { color3.r, color3.g, color3.b };
            }

            // Shininess
            float shininess = 0.0f;
            if (pMat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
            {
                material.shininess = shininess;
            }

            // Base color / Albedo (PBR)
            if (pMat->Get(AI_MATKEY_BASE_COLOR, color3) == AI_SUCCESS)
            {
                material.albedoColor = Vec3 { color3.r, color3.g, color3.b };
            }

            // Roughness
            float roughness = 0.0f;
            if (pMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
            {
                material.roughness = roughness;
            }

            // Metallic
            float metallic = 0.0f;
            if (pMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
            {
                material.metallic = metallic;
            }

            // Two sided
            int twoSided = 0;
            if (pMat->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS)
            {
                material.bDoubleSided = (twoSided != 0);
            }

            // Emissive
            if (pMat->Get(AI_MATKEY_COLOR_EMISSIVE, color3) == AI_SUCCESS)
            {
                material.emissiveColor = Vec3 { color3.r, color3.g, color3.b };
            }

            // emissive intensity
            float emissiveIntensity = 0.0f;
            if (pMat->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity) == AI_SUCCESS)
            {
                material.emissiveIntensity = emissiveIntensity;
            }

            // texture parsing
            if (bTextureEmbeded)
            {

                const Int32 numTexes                = NarrowCast<Int32>(pScene->mNumTextures);
                material.albedoTexIndex             = ParseMaterialTextureIndex(pMat, aiTextureType_DIFFUSE, numTexes);
                material.normalTexIndex             = ParseMaterialTextureIndex(pMat, aiTextureType_NORMALS, numTexes);
                material.metallicTexIndex           = ParseMaterialTextureIndex(pMat, aiTextureType_METALNESS, numTexes);
                material.roughnessTexIndex          = ParseMaterialTextureIndex(pMat, aiTextureType_DIFFUSE_ROUGHNESS, numTexes);
                material.aoTexIndex                 = ParseMaterialTextureIndex(pMat, aiTextureType_LIGHTMAP, numTexes);
                material.emissiveTexIndex           = ParseMaterialTextureIndex(pMat, aiTextureType_EMISSIVE, numTexes);
                material.lightTexIndex              = ParseMaterialTextureIndex(pMat, aiTextureType_LIGHTMAP, numTexes);
                material.vertexDisplacementTexIndex = ParseMaterialTextureIndex(pMat, aiTextureType_DISPLACEMENT, numTexes);

                // additional channel mapping
                {
                    aiString dummy;
                    if (pMat->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &dummy) == AI_SUCCESS)
                    {
                        material.roughnessTexChannel = eTextureChannel::Green;
                        material.metallicTexChannel  = eTextureChannel::Blue;
                    }
                }
            }

            m_materials.emplace_back(std::move(material));
        }
    }

    // build tree
    if (pScene->mRootNode)
    {
        m_rootNode = BuildImportedModelTree_Recursive(pScene->mRootNode, pScene);
    }

    return true;
}

void AssimpImporter::SetPath(const fs::path& _path)
{
    m_path = _path;
}

void AssimpImporter::ComputeTangent()
{
    m_bComputeTangent = true;
}

void AssimpImporter::ComputeNormal(const eNormalGenType _type)
{
    m_normalGenType = _type;
}

void AssimpImporter::DropExistingNormal()
{
    m_dropExistingNormal = true;
}

void AssimpImporter::LimitBoneWeight()
{
    m_bLimitBondWeights = true;
}

void AssimpImporter::LoadOptimizationForStaticMesh()
{
    m_bLoadStaticMesh = true;
}

const ImportedModelNode& AssimpImporter::GetRootNode() const
{
    return m_rootNode;
}

const std::vector<ImportedMesh>& AssimpImporter::GetMeshes() const
{
    return m_meshes;
}

const std::vector<ImportedMaterial>& AssimpImporter::GetMaterials() const
{
    return m_materials;
}

const std::vector<ImportedTexture>& AssimpImporter::GetTextures() const
{
    return m_textures;
}

}   // namespace jam