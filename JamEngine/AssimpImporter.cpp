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

[[nodiscard]] static Int32 TryTextureIndexFrom(   // << return k_invalidImportedIdx if failed
    const aiMaterial*   _pMat,
    const aiTextureType _type,
    const Int32         _maxIndex)
{
    // str 추출
    aiString str;
    if (!_pMat->GetTexture(_type, 0, &str) == AI_SUCCESS)
    {
        return k_invalidImportedIdx;
    }

    // 이름 파싱
    const StringView      rawName   = { str.C_Str(), NarrowCast<Int32>(str.length) };
    const Optional<Int32> optOffset = rawName.FindFirstOf('*');
    if (!optOffset)
    {
        return k_invalidImportedIdx;
    }

    // 인덱스 파싱
    const Int32      offset = optOffset.value();
    const StringView name   = rawName.SubView(offset + 1);

    // 인덱스 변환
    const Optional<Int32> optIdx = IntFrom<Int32>(name);
    if (!optIdx)
    {
        return k_invalidImportedIdx;
    }

    const Int32 idx = optIdx.value();
    return idx < _maxIndex ? idx : k_invalidImportedIdx;
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

    // 이름
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

    // 행렬
    {
        const aiMatrix4x4& aiMtx = _pNode->mTransformation;

        Mat4 mtx;
        mtx._11 = aiMtx.a1, mtx._12 = aiMtx.a2, mtx._13 = aiMtx.a3, mtx._14 = aiMtx.a4;
        mtx._21 = aiMtx.b1, mtx._22 = aiMtx.b2, mtx._23 = aiMtx.b3, mtx._24 = aiMtx.b4;
        mtx._31 = aiMtx.c1, mtx._32 = aiMtx.c2, mtx._33 = aiMtx.c3, mtx._34 = aiMtx.c4;
        mtx._41 = aiMtx.d1, mtx._42 = aiMtx.d2, mtx._43 = aiMtx.d3, mtx._44 = aiMtx.d4;
        node.mtx = mtx;
    }

    // 메쉬와 메쉬의 머테리얼 인덱스
    if (_pNode->mNumMeshes > 0)
    {
        node.meshIdxes.reserve(_pNode->mNumMeshes);
        node.materialIdxes.reserve(_pNode->mNumMeshes);

        // 메쉬는 반드시 하나의 머테리얼과 연관되어있음
        for (UINT i = 0; i < _pNode->mNumMeshes; ++i)
        {
            const UINT    mshIdx = _pNode->mMeshes[i];
            const aiMesh* pMesh  = _pScene->mMeshes[mshIdx];

            node.meshIdxes.emplace_back(mshIdx);
            node.materialIdxes.emplace_back(pMesh->mMaterialIndex);
        }
    }

    // 하위 자식 노드들
    const Int32 numChildren = NarrowCast<Int32>(_pNode->mNumChildren);
    node.children.reserve(numChildren);
    for (Int32 i = 0; i < numChildren; ++i)
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

    Vector<ImportedMesh> meshes;
    meshes.reserve(numMeshes);

    for (Int32 i = 0; i < numMeshes; ++i)
    {
        const aiMesh* pAiMesh  = _pScene->mMeshes[i];
        const Int32   numVerts = NarrowCast<Int32>(pAiMesh->mNumVertices);

        ImportedMesh mesh = {};

        // 이름
        {
            const aiString& aiName = pAiMesh->mName;
            if (aiName.Empty())
            {
                mesh.name = std::format("mesh_{}", i);
            }
            else
            {
                mesh.name = std::format("{}_{}", aiName.C_Str(), i);
            }
        }

        // 인덱스
        {
            const bool  bUseU32   = numVerts > 0xFFFF;
            const Int32 numFaces  = NarrowCast<Int32>(pAiMesh->mNumFaces);
            const Int32 numIdxes  = numFaces * 3;
            const Int32 byteWidth = CalcIndicesByteWidth(numIdxes, bUseU32);

            mesh.bUseU32 = bUseU32;
            mesh.idxData.resize(byteWidth);
            for (Int32 f = 0; f < numFaces; ++f)
            {
                const aiFace& aiFace = pAiMesh->mFaces[f];
                JAM_ASSERT(aiFace.mNumIndices == 3, "AssimpImporter::Import() - Face is not a triangle");
                for (Int32 p = 0; p < 3; ++p)
                {
                    StoreIndex(mesh.idxData, f * 3 + p, bUseU32, aiFace.mIndices[p]);
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
                    const Vec3 pos = Vec3From(pAiMesh->mVertices[v]);
                    mesh.positions.emplace_back(pos);
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
                    Vector<Vec4>& color = mesh.colors[j];
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
                mesh.normals.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    const Vec3 normal = Vec3From(pAiMesh->mNormals[v]);
                    mesh.normals.emplace_back(normal);
                }
            }
            else if (m_bGenerateNormals)   // 내장 normals가 없거나, 드롭된 경우
            {
                mesh.normals = ComputeNormals(mesh.positions, mesh.idxData, mesh.bUseU32);
            }
        }

        // tangents & bitangents
        {
            if (pAiMesh->HasTangentsAndBitangents() && !m_bDropExistingTangentFrame)
            {
                mesh.tangents.reserve(numVerts);
                mesh.bitangents.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    const Vec3 tangent   = Vec3From(pAiMesh->mTangents[v]);
                    const Vec3 bitangent = Vec3From(pAiMesh->mBitangents[v]);
                    mesh.tangents.emplace_back(tangent);
                    mesh.bitangents.emplace_back(bitangent);
                }
            }
            else if (m_bGenerateTangentFrame)   // 내장 tangent & bitangent가 없는 경우, 드롭된 경우
            {
                const TangentFrame tf = ComputeTangentFrame(mesh.positions, mesh.normals, mesh.texCoords[0], mesh.idxData, mesh.bUseU32);
                mesh.tangents         = std::move(tf.tangents);
                mesh.bitangents       = std::move(tf.bitangents);
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
    const Int32 numTextures = NarrowCast<Int32>(_pScene->mNumTextures);
    if (numTextures < 0)
    {
        return {};
    }

    Vector<ImportedTexture> textures;
    textures.reserve(numTextures);

    for (Int32 i = 0; i < numTextures; ++i)
    {
        const aiTexture* pAiTex        = _pScene->mTextures[i];
        const bool       bIsCompressed = (pAiTex->mHeight == 0);

        ImportedTexture tex = {};
        tex.width           = NarrowCast<Int32>(pAiTex->mWidth);
        tex.height          = NarrowCast<Int32>(pAiTex->mHeight);
        tex.bIsCompressed   = bIsCompressed;

        // pixel data
        if (bIsCompressed)
        {
            // 압축된 경우, format hint에 따라 포맷 결정
            tex.pixels = CMemView { pAiTex->pcData, tex.width };

            // 포맷 힌트로부터 이미지 포맷 결정
            const StringView             format    = { pAiTex->achFormatHint };
            const Optional<eImageFormat> optFormat = ImageFormatFrom(format);

            // 포맷 결정
            if (optFormat)
            {
                tex.format = optFormat.value();
            }
            else
            {
                // unknown format -> failed
                JAM_ASSERT(0, "AssimpImporter::ImportTexturesFrom_() - Unsupported embedded texture format: {}", format);
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

    Vector<ImportedMaterial> result;
    result.reserve(numMats);
    for (Int32 i = 0; i < numMats; ++i)
    {
        const aiMaterial* pMat     = _pScene->mMaterials[i];
        ImportedMaterial  material = {};

        // temp param
        aiString  strParam;
        aiColor3D colParam;
        float     fParam;
        int       iParam;

        // Name
        {
            if (pMat->Get(AI_MATKEY_NAME, strParam) == AI_SUCCESS)
            {
                material.name = std::format("{}_{}", strParam.C_Str(), i);
            }
            else
            {
                material.name = std::format("mat_{}", i);
            }
        }

        // Ambient
        if (pMat->Get(AI_MATKEY_COLOR_AMBIENT, colParam) == AI_SUCCESS)
        {
            material.ambientColor = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Diffuse
        if (pMat->Get(AI_MATKEY_COLOR_DIFFUSE, colParam) == AI_SUCCESS)
        {
            material.diffuseColor = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Specular
        if (pMat->Get(AI_MATKEY_COLOR_SPECULAR, colParam) == AI_SUCCESS)
        {
            material.specularColor = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Shininess
        if (pMat->Get(AI_MATKEY_SHININESS, fParam) == AI_SUCCESS)
        {
            material.shininess = fParam;
        }

        // Base color / Albedo (PBR)
        if (pMat->Get(AI_MATKEY_BASE_COLOR, colParam) == AI_SUCCESS)
        {
            material.albedoColor = Vec3 { colParam.r, colParam.g, colParam.b };
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
        if (pMat->Get(AI_MATKEY_COLOR_EMISSIVE, colParam) == AI_SUCCESS)
        {
            material.emissiveColor = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // emissive intensity
        if (pMat->Get(AI_MATKEY_EMISSIVE_INTENSITY, fParam) == AI_SUCCESS)
        {
            material.emissiveIntensity = fParam;
        }

        // texture parsing
        if (_pScene->mNumTextures > 0)
        {
            const Int32 numTextures           = NarrowCast<Int32>(_pScene->mNumTextures);
            material.albedoTexIdx             = TryTextureIndexFrom(pMat, aiTextureType_DIFFUSE, numTextures);
            material.normalTexIdx             = TryTextureIndexFrom(pMat, aiTextureType_NORMALS, numTextures);
            material.metallicTexIdx           = TryTextureIndexFrom(pMat, aiTextureType_METALNESS, numTextures);
            material.roughnessTexIdx          = TryTextureIndexFrom(pMat, aiTextureType_DIFFUSE_ROUGHNESS, numTextures);
            material.aoTexIdx                 = TryTextureIndexFrom(pMat, aiTextureType_LIGHTMAP, numTextures);
            material.emissiveTexIdx           = TryTextureIndexFrom(pMat, aiTextureType_EMISSIVE, numTextures);
            material.lightTexIdx              = TryTextureIndexFrom(pMat, aiTextureType_LIGHTMAP, numTextures);
            material.vertexDisplacementTexIdx = TryTextureIndexFrom(pMat, aiTextureType_DISPLACEMENT, numTextures);

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

bool AssimpImporter::Import(
    const fs::path& _path)
{
    CleanUp_();

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
    const aiScene* pAiScene = importer.ReadFile(_path.native(), flags);
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
    if (pAiScene->HasTextures())
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

void AssimpImporter::CleanUp_()
{
    m_rootNode = {};
    m_meshes.clear();
    m_materials.clear();
    m_textures.clear();
}

}   // namespace jam