#include "pch.h"

#include "AssimpImporter.h"

#include "BaseUtilities.h"
#include "MeshFactory.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// using global function because of fwd

namespace jam
{

[[nodiscard]] static Int32 TryTextureIndexFrom(
    const aiMaterial*   _pMat,
    const aiTextureType _type,
    const Int32         _maxIndex)
{
    aiString str;
    if (_pMat->GetTexture(_type, 0, &str) == AI_SUCCESS)
    {
        const StringView sv          = { str.C_Str(), NarrowCast<Int32>(str.length) };
        const StringView afterStarSv = sv.SubView(sv.FindFirstOf('*') + 1);

        if (const Optional<Int32> optIdx = IntFrom<Int32>(afterStarSv))
        {
            const Int32 idx = optIdx.value();
            return idx < _maxIndex ? idx : k_invalidImportedIdx;
        }
        // else: parsing failed
    }

    return k_invalidImportedIdx;   // this is legal
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

    // name
    {
        if (_pNode->mName.length > 0)
        {
            node.name = std::string(_pNode->mName.data, _pNode->mName.length);
        }
        else
        {
            node.name = "";
        }
    }

    // matrix
    {
        const aiMatrix4x4& aiMtx = _pNode->mTransformation;

        Mat4 mtx;
        mtx._11 = aiMtx.a1, mtx._12 = aiMtx.a2, mtx._13 = aiMtx.a3, mtx._14 = aiMtx.a4;
        mtx._21 = aiMtx.b1, mtx._22 = aiMtx.b2, mtx._23 = aiMtx.b3, mtx._24 = aiMtx.b4;
        mtx._31 = aiMtx.c1, mtx._32 = aiMtx.c2, mtx._33 = aiMtx.c3, mtx._34 = aiMtx.c4;
        mtx._41 = aiMtx.d1, mtx._42 = aiMtx.d2, mtx._43 = aiMtx.d3, mtx._44 = aiMtx.d4;
        node.mtx = mtx;
    }

    // meshes & materials
    if (_pNode->mNumMeshes > 0)
    {
        node.meshIdxes.reserve(_pNode->mNumMeshes);
        node.materialIdxes.reserve(_pNode->mNumMeshes);

        // mesh use single material
        for (UINT i = 0; i < _pNode->mNumMeshes; ++i)
        {
            const UINT    mshIdx = _pNode->mMeshes[i];
            const aiMesh* pMesh  = _pScene->mMeshes[mshIdx];

            node.meshIdxes.emplace_back(mshIdx);
            node.materialIdxes.emplace_back(pMesh->mMaterialIndex);
        }
    }

    // children
    UINT numChildren = _pNode->mNumChildren;
    node.children.reserve(numChildren);
    for (UINT i = 0; i < numChildren; ++i)
    {
        ImportedModelNode childNode = ImportedModelNodeFrom_Recursive(_pNode->mChildren[i], _pScene);
        node.children.emplace_back(std::move(childNode));
    }

    return node;
}

[[nodiscard]] static ImportedModelNode ImportedModelNodeFrom(
    const aiScene* _pScene)
{
    return ImportedModelNodeFrom_Recursive(_pScene->mRootNode, _pScene);
}

}   // namespace jam

namespace jam
{

Vector<ImportedMesh> AssimpImporter::ImportMeshesFrom_(
    const aiScene* _pScene) const
{
    const Int32 numMeshes = NarrowCast<Int32>(_pScene->mNumMeshes);
    if (numMeshes < 0)   // no mesh
    {
        return {};
    }

    const aiMesh* const* pAiMeshes = _pScene->mMeshes;

    Vector<ImportedMesh> result;
    result.reserve(numMeshes);

    for (Int32 i = 0; i < numMeshes; ++i)
    {
        const aiMesh* pAiMesh  = pAiMeshes[i];
        const Int32   numVerts = NarrowCast<Int32>(pAiMesh->mNumVertices);

        ImportedMesh importedMesh;

        // name
        {
            const aiString& aiName = pAiMesh->mName;
            if (aiName.Empty())
            {
                importedMesh.name = std::format("mesh_{}", i);
            }
            else
            {
                importedMesh.name = std::format("{}_{}", aiName.C_Str(), i);
            }
        }

        // indices
        {
            const bool  bUseU32   = numVerts > 0xFFFF;
            const Int32 idxStride = importedMesh.bUseU32 ? 4 : 2;
            const Int32 numFaces  = NarrowCast<Int32>(pAiMesh->mNumFaces);
            const Int32 numIdxes  = numFaces * 3;

            importedMesh.bUseU32 = bUseU32;
            importedMesh.idxData.resize(numIdxes * idxStride);
            for (Int32 f = 0; f < numFaces; ++f)
            {
                const aiFace& aiFace = pAiMesh->mFaces[f];
                JAM_ASSERT(aiFace.mNumIndices == 3, "AssimpImporter::Import() - Face is not a triangle");

                for (Int32 p = 0; p < 3; ++p)
                {
                    const Int32  offset = f * 3 + p;
                    const UInt32 idx    = aiFace.mIndices[p];
                    Byte*        pDst   = importedMesh.idxData.data() + idxStride * offset;

                    // write index
                    if (importedMesh.bUseU32)
                    {
                        UInt32* pDstU32 = reinterpret_cast<UInt32*>(pDst);
                        *pDstU32        = idx;
                    }
                    else
                    {
                        UInt16* pDstU16 = reinterpret_cast<UInt16*>(pDst);
                        *pDstU16        = NarrowCast<UInt16>(idx);
                    }
                }
            }
        }

        // positions
        {
            if (pAiMesh->HasPositions())
            {
                importedMesh.positions.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    const Vec3 pos = Vec3From(pAiMesh->mVertices[v]);
                    importedMesh.positions.emplace_back(pos);
                }
            }
        }

        // texture coordinates (up to 4)
        {
            for (Int32 j = 0; j < 4; ++j)
            {
                if (pAiMesh->HasTextureCoords(j))
                {
                    Vector<Vec2>& texCoord = importedMesh.texCoords[j];
                    texCoord.reserve(numVerts);
                    for (Int32 k = 0; k < numVerts; ++k)
                    {
                        const Vec2 uv = Vec2From(pAiMesh->mTextureCoords[j][k]);
                        texCoord.emplace_back(uv);
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
                    Vector<Vec4>& color = importedMesh.colors[j];
                    color.reserve(numVerts);
                    for (Int32 k = 0; k < numVerts; ++k)
                    {
                        const Vec4 col = Vec4From(pAiMesh->mColors[j][k]);
                        color.emplace_back(col);
                    }
                }
            }
        }

        // normals
        // m_bGenerateNormals 가 true 여도. mesh에 normal이 있으면 그것을 우선 사용한다.
        {
            if (pAiMesh->HasNormals() && !m_bDropExistingNormals)
            {
                importedMesh.normals.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    const Vec3 normal = Vec3From(pAiMesh->mNormals[v]);
                    importedMesh.normals.emplace_back(normal);
                }
            }
            else if (m_bGenerateNormals)   // 내장 normals가 없거나, 드롭된 경우
            {
                importedMesh.normals = ComputeNormals(importedMesh.positions, importedMesh.idxData, importedMesh.bUseU32);
            }
        }

        // tangents & bitangents
        {
            if (pAiMesh->HasTangentsAndBitangents() && !m_bDropExistingTangentFrame)
            {
                importedMesh.tangents.reserve(numVerts);
                importedMesh.bitangents.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    const Vec3 tangent   = Vec3From(pAiMesh->mTangents[v]);
                    const Vec3 bitangent = Vec3From(pAiMesh->mBitangents[v]);
                    importedMesh.tangents.emplace_back(tangent);
                    importedMesh.bitangents.emplace_back(bitangent);
                }
            }
            else if (m_bGenerateTangentFrame)   // 내장 tangent & bitangent가 없는 경우, 드롭된 경우
            {
                const TangentFrame tf   = ComputeTangentFrame(importedMesh.positions, importedMesh.normals, importedMesh.texCoords[0], importedMesh.idxData, importedMesh.bUseU32);
                importedMesh.tangents   = std::move(tf.tangents);
                importedMesh.bitangents = std::move(tf.bitangents);
            }
        }

        // create ImportedMesh
        result.emplace_back(std::move(importedMesh));
    }

    return result;
}

Vector<ImportedTexture> AssimpImporter::ImportTexturesFrom_(
    const aiScene* _pScene) const
{
    const Int32 numTexes = NarrowCast<Int32>(_pScene->mNumTextures);
    if (numTexes < 0)
    {
        return {};
    }

    const aiTexture* const* pAiTexes = _pScene->mTextures;

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

        // pixel data
        if (bIsCompressed)
        {
            // 압축된 경우, format hint에 따라 포맷 결정
            const Int32 byteWidth = tex.width;
            const Byte* pPixel    = reinterpret_cast<const Byte*>(pAiTex->pcData);
            tex.pixels            = CMemView { pPixel, byteWidth };

            // 포맷 힌트로부터 이미지 포맷 결정
            const StringView             formatHint = { pAiTex->achFormatHint };
            const Optional<eImageFormat> optFormat  = ImageFormatFrom(formatHint);

            if (optFormat)
            {
                tex.format = optFormat.value();
            }
            else
            {
                // unknown format -> failed
                JAM_ASSERT(0, "AssimpImporter::ImportTexturesFrom_() - Unsupported embedded texture format: {}", formatHint);
                tex            = {};
                tex.bTombstone = true;
            }
        }
        else
        {
            // 비압축된 경우, RGBA8 포맷 고정
            const Int32 byteWidth = NarrowCast<Int32>(tex.width * tex.height * sizeof(aiTexel));
            tex.pixels            = CMemView { pAiTex->pcData, byteWidth };
        }

        textures.emplace_back(std::move(tex));
    }

    return textures;
}

Vector<ImportedMaterial> AssimpImporter::ImportMaterialsFrom_(
    const aiScene* _pScene) const
{
    const Int32 numMats = NarrowCast<Int32>(_pScene->mNumMaterials);
    if (numMats < 0)
    {
        return {};
    }

    aiMaterial* const* pMats = _pScene->mMaterials;

    Vector<ImportedMaterial> result;
    result.reserve(numMats);

    for (Int32 i = 0; i < numMats; ++i)
    {
        const aiMaterial* pMat = pMats[i];

        ImportedMaterial material;
        aiString         strParam;
        aiColor3D        color3Param;
        float            fParam;
        int              iParam;

        // Name
        {
            if (pMat->Get(AI_MATKEY_NAME, strParam) == AI_SUCCESS && !strParam.Empty())
            {
                material.name = std::format("{}_{}", strParam.C_Str(), i);
            }
            else
            {
                material.name = std::format("mat_{}", i);
            }
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
            const Int32 numTexes              = NarrowCast<Int32>(_pScene->mNumTextures);
            material.albedoTexIdx             = TryTextureIndexFrom(pMat, aiTextureType_DIFFUSE, numTexes);
            material.normalTexIdx             = TryTextureIndexFrom(pMat, aiTextureType_NORMALS, numTexes);
            material.metallicTexIdx           = TryTextureIndexFrom(pMat, aiTextureType_METALNESS, numTexes);
            material.roughnessTexIdx          = TryTextureIndexFrom(pMat, aiTextureType_DIFFUSE_ROUGHNESS, numTexes);
            material.aoTexIdx                 = TryTextureIndexFrom(pMat, aiTextureType_LIGHTMAP, numTexes);
            material.emissiveTexIdx           = TryTextureIndexFrom(pMat, aiTextureType_EMISSIVE, numTexes);
            material.lightTexIdx              = TryTextureIndexFrom(pMat, aiTextureType_LIGHTMAP, numTexes);
            material.vertexDisplacementTexIdx = TryTextureIndexFrom(pMat, aiTextureType_DISPLACEMENT, numTexes);

            // additional channel mapping
            if (pMat->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &strParam) == AI_SUCCESS)
            {
                material.roughnessTexChannel = eTextureChannel::Green;
                material.metallicTexChannel  = eTextureChannel::Blue;
            }
        }

        result.emplace_back(std::move(material));
    }

    return result;
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
    const aiScene* pAiScene = importer.ReadFile(m_path.native(), flags);
    if (!pAiScene)
    {
        GlobalBuffer::SetLastError("failed to load file: {}", importer.GetErrorString());
        return false;
    }

    // mesh load
    if (pAiScene->HasMeshes())
    {
        m_meshes = ImportMeshesFrom_(pAiScene);
    }

    // texture load
    if (pAiScene->mNumTextures > 0)
    {
        m_textures = ImportTexturesFrom_(pAiScene);
    }

    // material load
    if (pAiScene->HasMaterials())
    {
        m_materials = ImportMaterialsFrom_(pAiScene);
    }

    // build tree
    if (pAiScene->mRootNode)
    {
        m_rootNode = ImportedModelNodeFrom(pAiScene);
    }

    return true;
}

void AssimpImporter::SetPath(
    const fs::path& _path)
{
    m_path = _path;
}

void AssimpImporter::GenerateTangentFrame(
    const bool _bDropExistTangents)
{
    m_bGenerateTangentFrame     = true;
    m_bDropExistingTangentFrame = _bDropExistTangents;
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