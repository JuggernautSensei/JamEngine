#include "pch.h"

#include "AssimpImporter.h"

#include "BaseUtilities.h"
#include "MeshFactory.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace jam
{

[[nodiscard]] static Int32 TryParseMaterialTextureIndex(
    const aiMaterial*   _pMat,
    const aiTextureType _type,
    const Int32         _boundaryIndex)
{
    aiString str;
    if (_pMat->GetTexture(_type, 0, &str) == AI_SUCCESS)
    {
        const char* c = str.C_Str() + 1;   // skip '*'
        if (const std::optional<Int32> opt = TryParseStringToInt<Int32>(c))
        {
            const Int32 index = *opt;
            return (index >= 0 && index < _boundaryIndex) ? index : k_invalidImportedTexIndex;
        }
        else
        {
            return k_invalidImportedTexIndex;
        }
    }
    else
    {
        return k_invalidImportedTexIndex;
    }
}

[[nodiscard]] static Vec2 Vec2From(
    const aiVector3D _vec)
{
    return Vec2 { _vec.x, _vec.y };
}

[[nodiscard]] static Vec3 Vec3From(
    const aiVector3D _vec)
{
    return Vec3 { _vec.x, _vec.y, _vec.z };
}

[[nodiscard]] static Vec4 Vec4From(
    const aiColor4D _color)
{
    return Vec4 { _color.r, _color.g, _color.b, _color.a };
}

[[nodiscard]] static ImportedModelNode ImportedModelNodeFrom_Recursive(
    const aiNode*  _pNode,
    const aiScene* _pScene)
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
    node.mtx = mat;

    if (_pNode->mNumMeshes > 0)
    {
        for (UINT i = 0; i < _pNode->mNumMeshes; ++i)
        {
            const UINT    meshIndex = _pNode->mMeshes[i];
            const aiMesh* pMesh     = _pScene->mMeshes[meshIndex];

            node.meshIndices.emplace_back(meshIndex);
            node.materialIndices.emplace_back(pMesh->mMaterialIndex);
        }
    }

    // children
    UINT numChildren = _pNode->mNumChildren;
    node.children.reserve(numChildren);
    for (UINT i = 0; i < numChildren; ++i)
    {
        node.children.emplace_back(ImportedModelNodeFrom_Recursive(_pNode->mChildren[i], _pScene));
    }

    return node;
}

}   // namespace jam

namespace jam
{

Vector<ImportedMesh> AssimpImporter::ImportMeshesFrom_(
    const aiScene* _pScene) const
{
    const Int32          numMeshes = NarrowCast<Int32>(_pScene->mNumMeshes);
    const aiMesh* const* pAiMeshes = _pScene->mMeshes;
    JAM_ASSERT(numMeshes > 0, "AssimpImporter::ImportMeshesFrom_() - No mesh found");

    Vector<ImportedMesh> meshes;
    meshes.reserve(numMeshes);
    for (Int32 i = 0; i < numMeshes; ++i)
    {
        const aiMesh* pAiMesh  = pAiMeshes[i];
        const Int32   numVerts = NarrowCast<Int32>(pAiMesh->mNumVertices);

        ImportedMesh mesh;

        // name
        {
            if (pAiMesh->mName.length > 0)
            {
                mesh.name = std::format("{}_{}", pAiMesh->mName.C_Str(), i);
            }
            else
            {
                mesh.name = std::format("mesh_{}", i);
            }
        }

        // indices
        {
            mesh.bUseU32           = numVerts > 0xFFFF;
            const Int32 idxStride  = mesh.bUseU32 ? 4 : 2;
            const Int32 numFaces   = NarrowCast<Int32>(pAiMesh->mNumFaces);
            const Int32 numIndices = numFaces * 3;

            mesh.idxData.resize(numIndices * idxStride);
            for (Int32 j = 0; j < numFaces; ++j)
            {
                const aiFace& aiFace = pAiMesh->mFaces[j];
                JAM_ASSERT(aiFace.mNumIndices == 3, "AssimpImporter::Import() - Face is not a triangle");

                for (Int32 k = 0; k < 3; ++k)
                {
                    const Int32 offset = j * 3 + k;
                    const Int32 index  = NarrowCast<Int32>(aiFace.mIndices[k]);
                    Byte*       pDst   = mesh.idxData.data() + idxStride * offset;

                    // write index
                    if (mesh.bUseU32)
                    {
                        *pDst = index;
                    }
                    else
                    {
                        *pDst = NarrowCast<UInt16>(index);
                    }
                }
            }
        }

        // positions
        {
            if (pAiMesh->HasPositions())
            {
                mesh.positions.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    mesh.positions.emplace_back(Vec3From(pAiMesh->mVertices[v]));
                }
            }
        }

        // texture coordinates (up to 4)
        {
            for (Int32 j = 0; j < 4; ++j)
            {
                if (pAiMesh->HasTextureCoords(j))
                {
                    Vector<Vec2>& texCoord = mesh.texCoords[j];
                    texCoord.reserve(numVerts);
                    for (Int32 k = 0; k < numVerts; ++k)
                    {
                        texCoord.emplace_back(Vec2From(pAiMesh->mTextureCoords[j][k]));
                    }
                }
            }
        }

        // colors (up to 4)
        {
            for (Int32 j = 0; j < 4; ++j)
            {
                if (pAiMesh->HasVertexColors(j))
                {
                    Vector<Vec4>& color = mesh.colors[j];
                    color.reserve(numVerts);
                    for (Int32 k = 0; k < numVerts; ++k)
                    {
                        color.emplace_back(Vec4From(pAiMesh->mColors[j][k]));
                    }
                }
            }
        }

        // normals
        // m_bGenerateNormals 가 true 여도. mesh에 normal이 있으면 그것을 우선 사용한다.
        {
            if (pAiMesh->HasNormals() && !m_bDropExistingNormals)
            {
                mesh.normals.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    mesh.normals.emplace_back(Vec3From(pAiMesh->mNormals[v]));
                }
            }
            else if (m_bGenerateNormals)   // 내장 normals가 없거나, 드롭된 경우
            {
                mesh.normals = ComputeNormals(mesh.positions, mesh.idxData, mesh.bUseU32);
            }
        }

        // tangents & bitangents
        {
            if (pAiMesh->HasTangentsAndBitangents())
            {
                mesh.tangents.reserve(numVerts);
                mesh.bitangents.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    mesh.tangents.emplace_back(Vec3From(pAiMesh->mTangents[v]));
                    mesh.bitangents.emplace_back(Vec3From(pAiMesh->mBitangents[v]));
                }
            }
            else if (m_bGenerateTangentFrame)
            {
                const TangentFrame frame = ComputeTangentFrame(mesh.positions, mesh.normals, mesh.texCoords[0], mesh.idxData, mesh.bUseU32);
                mesh.tangents            = std::move(frame.tangents);
                mesh.bitangents          = std::move(frame.bitangents);
            }
        }

        // create ImportedMesh
        meshes.emplace_back(std::move(mesh));
    }

    return meshes;
}

Vector<ImportedTexture> AssimpImporter::ImportTexturesFrom_(
    const aiScene* _pScene) const
{
    aiTexture* const* pAiTexes = _pScene->mTextures;
    const Int32       numTexes = NarrowCast<Int32>(_pScene->mNumTextures);
    JAM_ASSERT(numTexes > 0, "AssimpImporter::ImportTexturesFrom_() - No texture found");

    Vector<ImportedTexture> textures;
    textures.reserve(numTexes);
    for (Int32 i = 0; i < numTexes; ++i)
    {
        const aiTexture* pAiTex        = pAiTexes[i];
        const bool       bIsCompressed = (pAiTex->mHeight == 0);

        ImportedTexture tex;
        tex.width         = NarrowCast<Int32>(pAiTex->mWidth);
        tex.height        = NarrowCast<Int32>(pAiTex->mHeight);
        tex.bIsCompressed = bIsCompressed;

        if (bIsCompressed)
        {
            // Compressed texture
            const Int32            byteWidth  = tex.width;
            const Byte*            pPixel     = reinterpret_cast<const Byte*>(pAiTex->pcData);
            const std::string_view formatHint = { pAiTex->achFormatHint, strlen(pAiTex->achFormatHint) };

            tex.pixels = ConstMemView { pPixel, static_cast<size_t>(byteWidth) };
            tex.format = ImageFormatFrom(formatHint);
        }
        else
        {
            // BGRA 로 가정
            const size_t byteWidth = tex.width * tex.height * sizeof(aiTexel);
            const Byte*  pPixel    = reinterpret_cast<const Byte*>(pAiTex->pcData);
            tex.pixels             = ConstMemView { pPixel, byteWidth };
        }

        textures.emplace_back(std::move(tex));
    }

    return textures;
}

Vector<ImportedMaterial> AssimpImporter::ImportMaterialsFrom_(
    const aiScene* _pScene) const
{
    aiMaterial* const* pMats   = _pScene->mMaterials;
    const Int32        numMats = NarrowCast<Int32>(_pScene->mNumMaterials);
    JAM_ASSERT(numMats > 0, "AssimpImporter::ImportMaterialsFrom_() - No material found");

    Vector<ImportedMaterial> materials;
    materials.reserve(numMats);
    for (Int32 i = 0; i < numMats; ++i)
    {
        const aiMaterial* pMat = pMats[i];
        ImportedMaterial  material;

        aiString  strParam;
        aiColor3D color3Param;
        float     fParam;
        int       iParam;

        // Name
        if (pMat->Get(AI_MATKEY_NAME, strParam) == AI_SUCCESS && strParam.length > 0)
        {
            material.name = std::format("{}_{}", strParam.C_Str(), i);
        }
        else
        {
            material.name = std::format("mat_{}", i);
        }

        // Ambient
        if (pMat->Get(AI_MATKEY_COLOR_AMBIENT, color3Param) == AI_SUCCESS)
        {
            material.ambientColor = Vec3 { color3Param.r, color3Param.g, color3Param.b };
        }

        // Diffuse
        if (pMat->Get(AI_MATKEY_COLOR_DIFFUSE, color3Param) == AI_SUCCESS)
        {
            material.diffuseColor = Vec3 { color3Param.r, color3Param.g, color3Param.b };
        }

        // Specular
        if (pMat->Get(AI_MATKEY_COLOR_SPECULAR, color3Param) == AI_SUCCESS)
        {
            material.specularColor = Vec3 { color3Param.r, color3Param.g, color3Param.b };
        }

        // Shininess
        if (pMat->Get(AI_MATKEY_SHININESS, fParam) == AI_SUCCESS)
        {
            material.shininess = fParam;
        }

        // Base color / Albedo (PBR)
        if (pMat->Get(AI_MATKEY_BASE_COLOR, color3Param) == AI_SUCCESS)
        {
            material.albedoColor = Vec3 { color3Param.r, color3Param.g, color3Param.b };
        }

        // Roughness
        if (pMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, fParam) == AI_SUCCESS)
        {
            material.roughness = fParam;
        }

        // Metallic
        if (pMat->Get(AI_MATKEY_METALLIC_FACTOR, fParam) == AI_SUCCESS)
        {
            material.metallic = fParam;
        }

        // Two sided
        if (pMat->Get(AI_MATKEY_TWOSIDED, iParam) == AI_SUCCESS)
        {
            material.bDoubleSided = (iParam != 0);
        }

        // Emissive
        if (pMat->Get(AI_MATKEY_COLOR_EMISSIVE, color3Param) == AI_SUCCESS)
        {
            material.emissiveColor = Vec3 { color3Param.r, color3Param.g, color3Param.b };
        }

        // emissive intensity
        if (pMat->Get(AI_MATKEY_EMISSIVE_INTENSITY, fParam) == AI_SUCCESS)
        {
            material.emissiveIntensity = fParam;
        }

        // texture parsing
        if (_pScene->mNumTextures > 0)
        {
            const Int32 numTexes                = NarrowCast<Int32>(_pScene->mNumTextures);
            material.albedoTexIndex             = TryParseMaterialTextureIndex(pMat, aiTextureType_DIFFUSE, numTexes);
            material.normalTexIndex             = TryParseMaterialTextureIndex(pMat, aiTextureType_NORMALS, numTexes);
            material.metallicTexIndex           = TryParseMaterialTextureIndex(pMat, aiTextureType_METALNESS, numTexes);
            material.roughnessTexIndex          = TryParseMaterialTextureIndex(pMat, aiTextureType_DIFFUSE_ROUGHNESS, numTexes);
            material.aoTexIndex                 = TryParseMaterialTextureIndex(pMat, aiTextureType_LIGHTMAP, numTexes);
            material.emissiveTexIndex           = TryParseMaterialTextureIndex(pMat, aiTextureType_EMISSIVE, numTexes);
            material.lightTexIndex              = TryParseMaterialTextureIndex(pMat, aiTextureType_LIGHTMAP, numTexes);
            material.vertexDisplacementTexIndex = TryParseMaterialTextureIndex(pMat, aiTextureType_DISPLACEMENT, numTexes);

            // additional channel mapping
            if (pMat->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &strParam) == AI_SUCCESS)
            {
                material.roughnessTexChannel = eTextureChannel::Green;
                material.metallicTexChannel  = eTextureChannel::Blue;
            }
        }

        materials.emplace_back(std::move(material));
    }

    return materials;
}

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

    if (m_bDropExistingNormals)
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

    Assimp::Importer importer;
    if (m_bLoadStaticMesh)
    {
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS);
    }
    else
    {
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS | aiComponent_BONEWEIGHTS | aiComponent_ANIMATIONS);
    }

    // remove line & point primitives
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);

    // scene load
    const aiScene* pScene = importer.ReadFile(m_path.native(), flags);
    if (!pScene)
    {
        ErrorBuffer::Set("failed to load file: {}", importer.GetErrorString());
        return false;
    }

    // mesh load
    if (pScene->HasMeshes())
    {
        m_meshes = ImportMeshesFrom_(pScene);
    }

    // texture load
    if (pScene->mNumTextures > 0)
    {
        m_textures = ImportTexturesFrom_(pScene);
    }

    // material load
    if (pScene->HasMaterials())
    {
        m_materials = ImportMaterialsFrom_(pScene);
    }

    // build tree
    if (pScene->mRootNode)
    {
        m_rootNode = ImportedModelNodeFrom_Recursive(pScene->mRootNode, pScene);
    }

    return true;
}

void AssimpImporter::SetPath(
    const fs::path& _path)
{
    m_path = _path;
}

void AssimpImporter::GenerateTangentFrame()
{
    m_bGenerateTangentFrame = true;
}

void AssimpImporter::GenerateNormals(
    const bool _bDropExistNormals)
{
    m_bGenerateNormals     = true;
    m_bDropExistingNormals = _bDropExistNormals;
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

const Vector<ImportedMesh>& AssimpImporter::GetMeshes() const
{
    return m_meshes;
}

const Vector<ImportedMaterial>& AssimpImporter::GetMaterials() const
{
    return m_materials;
}

const Vector<ImportedTexture>& AssimpImporter::GetTextures() const
{
    return m_textures;
}

}   // namespace jam