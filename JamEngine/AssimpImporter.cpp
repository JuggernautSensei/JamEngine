#include "pch.h"

#include "AssimpImporter.h"

#include "BaseUtilities.h"
#include "MeshFactory.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "GeometryUtilities.h"
#include "Graphics.h"
#include "ImageImporter.h"
#include "Model.h"

// using global function because of fwd

namespace jam
{

[[nodiscard]] static Int32 ParseTextureIndexOrInvalidIndex(   // << return k_invalidImportedIdx if failed
    const aiMaterial*   _pMat,
    const aiTextureType _type)
{
    // 전체 네임 추출
    aiString fullName = {};
    if (!_pMat->GetTexture(_type, 0, &fullName) == AI_SUCCESS)
    {
        return k_invalidImportedIdx;
    }

    // 이름 파싱 (이상적인 이름은 *{index} 형태)
    const StringView name = { fullName.C_Str(), NarrowCast<Int32>(fullName.length) };
    if (!name.StartWith("*"))
    {
        return k_invalidImportedIdx;
    }

    // 인덱스 변환
    const StringView strIdx = name.SubView(1);
    Int32            idx    = 0;
    if (!MoveIfHasValue(IntFrom<Int32>(strIdx), idx))
    {
        return k_invalidImportedIdx;
    }

    return idx;
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

[[nodiscard]] static ImportedModelNode CreateImportedModelNode_Recursive(
    const aiNode*  _pNode,
    const aiScene* _pScene)
{
    ImportedModelNode node = {};

    // 이름
    node.name = _pNode->mName.C_Str();

    // 행렬
    {
        const aiMatrix4x4& aiMtx = _pNode->mTransformation;

        Mat4 mtx = {};
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
        node.matIdxes.reserve(_pNode->mNumMeshes);

        // 메쉬는 반드시 하나의 머테리얼과 연관되어있음
        for (UINT i = 0; i < _pNode->mNumMeshes; ++i)
        {
            const UINT    mshIdx = _pNode->mMeshes[i];
            const aiMesh* pMesh  = _pScene->mMeshes[mshIdx];

            node.meshIdxes.emplace_back(mshIdx);
            node.matIdxes.emplace_back(pMesh->mMaterialIndex);
        }
    }

    // 하위 자식 노드들
    const Int32 numChildren = NarrowCast<Int32>(_pNode->mNumChildren);
    node.children.reserve(numChildren);
    for (Int32 i = 0; i < numChildren; ++i)
    {
        ImportedModelNode childNode = CreateImportedModelNode_Recursive(_pNode->mChildren[i], _pScene);
        node.children.emplace_back(std::move(childNode));
    }

    return node;
}

}   // namespace jam

namespace jam
{

Vector<ImportedMesh> AssimpImporter::CreateImportedMeshes_(
    aiMesh**    _ppMesh,
    const Int32 _numMeshes) const
{
    JAM_ASSERT(_ppMesh != nullptr, "AssimpImporter::ImportMeshesFrom_() - _pMesh is nullptr");
    JAM_ASSERT(_numMeshes > 0, "AssimpImporter::ImportMeshesFrom_() - _numAiMeshes is zero or negative");

    Vector<ImportedMesh> meshes = {};
    meshes.reserve(_numMeshes);

    // 메쉬 순회
    for (Int32 i = 0; i < _numMeshes; ++i)
    {
        const aiMesh* pMesh    = _ppMesh[i];
        const Int32   numVerts = NarrowCast<Int32>(pMesh->mNumVertices);

        ImportedMesh mesh = {};

        // 이름
        {
            const aiString& aiName = pMesh->mName;
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
            // 인텍스 포맷
            const bool bUseU32 = IsGreaterThanUInt16(numVerts);
            mesh.bUseU32       = bUseU32;

            // 인덱스 버퍼 할당
            const Int32  numFaces  = NarrowCast<Int32>(pMesh->mNumFaces);
            const Int32  numIdxes  = numFaces * 3;
            const Int32  byteWidth = CalcIndicesByteWidth(numIdxes, bUseU32);
            Vector<Byte> idxData(byteWidth);

            // 인덱스 작성
            IndexWriter indexWriter { idxData, bUseU32 };
            for (Int32 f = 0; f < numFaces; ++f)
            {
                const aiFace& aiFace = pMesh->mFaces[f];
                JAM_ASSERT(aiFace.mNumIndices == 3, "AssimpImporter::Import() - Face is not a triangle");
                for (Int32 p = 0; p < 3; ++p)
                {
                    indexWriter.WriteAt(f * 3 + p, aiFace.mIndices[p]);
                }
            }

            // mesh로 이동
            mesh.idxData = std::move(idxData);
        }

        // positions
        {
            if (pMesh->HasPositions())
            {
                mesh.positions.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    const Vec3 pos = Vec3From(pMesh->mVertices[v]);
                    mesh.positions.emplace_back(pos);
                }
            }
        }

        // texture coordinates (up to 4)
        {
            for (Int32 j = 0; j < 4; ++j)
            {
                if (pMesh->HasTextureCoords(j))
                {
                    Vector<Vec2>& texCoord = mesh.texCoords[j];
                    texCoord.reserve(numVerts);
                    for (Int32 k = 0; k < numVerts; ++k)
                    {
                        const Vec2 uv = Vec2From(pMesh->mTextureCoords[j][k]);
                        texCoord.emplace_back(uv);
                    }
                }
            }
        }

        // colors (up to 4)
        {
            for (Int32 j = 0; j < 4; ++j)
            {
                if (pMesh->HasVertexColors(j))
                {
                    Vector<Vec4>& color = mesh.colors[j];
                    color.reserve(numVerts);
                    for (Int32 k = 0; k < numVerts; ++k)
                    {
                        const Vec4 col = Vec4From(pMesh->mColors[j][k]);
                        color.emplace_back(col);
                    }
                }
            }
        }

        // normals
        // m_options.bGenerateNormals 가 true 여도. mesh에 normal이 있으면 그것을 우선 사용한다.
        {
            if (pMesh->HasNormals() && !m_options.bDropExistingNormals)
            {
                mesh.normals.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    const Vec3 normal = Vec3From(pMesh->mNormals[v]);
                    mesh.normals.emplace_back(normal);
                }
            }
            else if (m_options.bGenerateNormals)   // 내장 normals가 없거나, 드롭된 경우
            {
                mesh.normals = ComputeNormals(mesh.positions, mesh.idxData, mesh.bUseU32);
            }
        }

        // tangents & bitangents
        {
            if (pMesh->HasTangentsAndBitangents() && !m_options.bDropExistingTangentFrame)
            {
                mesh.tangents.reserve(numVerts);
                mesh.bitangents.reserve(numVerts);
                for (Int32 v = 0; v < numVerts; ++v)
                {
                    const Vec3 tangent   = Vec3From(pMesh->mTangents[v]);
                    const Vec3 bitangent = Vec3From(pMesh->mBitangents[v]);
                    mesh.tangents.emplace_back(tangent);
                    mesh.bitangents.emplace_back(bitangent);
                }
            }
            else if (m_options.bGenerateTangentFrame)   // 내장 tangent & bitangent가 없는 경우, 드롭된 경우
            {
                const TangentFrame tangentFrame = ComputeTangentFrame(mesh.positions, mesh.normals, mesh.texCoords[0], mesh.idxData, mesh.bUseU32);

                mesh.tangents   = std::move(tangentFrame.tangents);
                mesh.bitangents = std::move(tangentFrame.bitangents);
            }
        }

        // create ImportedMesh
        meshes.emplace_back(std::move(mesh));
    }

    return meshes;
}

Vector<ImportedTexture> AssimpImporter::CreateImportedTextures_(
    aiTexture** _pTexture,
    const Int32 _numTextures) const
{
    Vector<ImportedTexture> textures = {};
    textures.reserve(_numTextures);

    // 텍스처 순회
    for (Int32 i = 0; i < _numTextures; ++i)
    {
        const aiTexture* pTexture      = _pTexture[i];
        const bool       bIsCompressed = (pTexture->mHeight == 0);   // 압축된 텍스처는 mHeight가 0

        // 기본 정보
        ImportedTexture tex = {};
        tex.width           = NarrowCast<Int32>(pTexture->mWidth);
        tex.height          = NarrowCast<Int32>(pTexture->mHeight);
        tex.bIsCompressed   = bIsCompressed;

        // 픽셀 데이터
        if (bIsCompressed)
        {
            // 압축된 경우, format hint에 따라 포맷 결정
            tex.pixels = CMemView { pTexture->pcData, tex.width };

            // 포맷 결정
            const StringView format = { pTexture->achFormatHint };
            if (!MoveIfHasValue(ImageFormatFrom(format), tex.format))
            {
                // 포맷 획득 실패시 경고 및 플레이스홀더 텍스처로 대체
                JAM_ASSERT(0, "AssimpImporter::ImportTexturesFrom_() - Unsupported embedded texture format: {}", format);
                tex              = {};
                tex.bPlaceholder = true;   // placeholder for failed texture
            }
        }
        else
        {
            // 비압축된 경우, RGBA8 포맷 고정
            const Int32 byteWidth = NarrowCast<Int32>(tex.width * tex.height * sizeof(aiTexel));
            tex.pixels            = CMemView { pTexture->pcData, byteWidth };
        }

        // done
        textures.emplace_back(tex);
    }

    return textures;
}

Vector<ImportedMaterial> AssimpImporter::CreateImportedMaterials_(
    aiMaterial** _pMats,
    const Int32  _numMats) const
{
    JAM_ASSERT(_pMats != nullptr, "AssimpImporter::ImportMaterialsFrom_() - _pMats is nullptr");
    JAM_ASSERT(_numMats > 0, "AssimpImporter::ImportMaterialsFrom_() - _numMats is zero or negative");

    Vector<ImportedMaterial> result;
    result.reserve(_numMats);

    // 머테리얼 순회
    for (Int32 i = 0; i < _numMats; ++i)
    {
        const aiMaterial* pMat     = _pMats[i];
        ImportedMaterial  material = {};

        // out params receiver
        aiString  strParam = {};
        aiColor3D colParam = {};
        float     fParam   = 0.f;
        int       iParam   = 0;

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
            material.params.ambient = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Diffuse
        if (pMat->Get(AI_MATKEY_COLOR_DIFFUSE, colParam) == AI_SUCCESS)
        {
            material.params.diffuse = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Specular
        if (pMat->Get(AI_MATKEY_COLOR_SPECULAR, colParam) == AI_SUCCESS)
        {
            material.params.specular = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Shininess
        if (pMat->Get(AI_MATKEY_SHININESS, fParam) == AI_SUCCESS)
        {
            material.params.shininess = fParam;
        }

        // Base color / Albedo (PBR)
        if (pMat->Get(AI_MATKEY_BASE_COLOR, colParam) == AI_SUCCESS)
        {
            material.params.albedo = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Roughness
        if (pMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, fParam) == AI_SUCCESS)
        {
            material.params.roughness = fParam;
        }

        // Metallic
        if (pMat->Get(AI_MATKEY_METALLIC_FACTOR, fParam) == AI_SUCCESS)
        {
            material.params.metallic = fParam;
        }

        // Two sided
        if (pMat->Get(AI_MATKEY_TWOSIDED, iParam) == AI_SUCCESS)
        {
            material.params.bDoubleSided = (iParam != 0);
        }

        // Emissive
        if (pMat->Get(AI_MATKEY_COLOR_EMISSIVE, colParam) == AI_SUCCESS)
        {
            material.params.emissive = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // emissive intensity
        if (pMat->Get(AI_MATKEY_EMISSIVE_INTENSITY, fParam) == AI_SUCCESS)
        {
            material.params.emissiveIntensity = fParam;
        }

        // Mapture parsing
        material.textures.Set(eMaterialTextureType::Albedo, ParseTextureIndexOrInvalidIndex(pMat, aiTextureType_DIFFUSE));
        material.textures.Set(eMaterialTextureType::Ao, ParseTextureIndexOrInvalidIndex(pMat, aiTextureType_AMBIENT_OCCLUSION));
        material.textures.Set(eMaterialTextureType::Normal, ParseTextureIndexOrInvalidIndex(pMat, aiTextureType_NORMALS));
        material.textures.Set(eMaterialTextureType::Metallic, ParseTextureIndexOrInvalidIndex(pMat, aiTextureType_METALNESS));
        material.textures.Set(eMaterialTextureType::Roughness, ParseTextureIndexOrInvalidIndex(pMat, aiTextureType_DIFFUSE_ROUGHNESS));
        material.textures.Set(eMaterialTextureType::Light, ParseTextureIndexOrInvalidIndex(pMat, aiTextureType_LIGHTMAP));
        material.textures.Set(eMaterialTextureType::Emissive, ParseTextureIndexOrInvalidIndex(pMat, aiTextureType_EMISSIVE));
        material.textures.Set(eMaterialTextureType::VertexDisplacement, ParseTextureIndexOrInvalidIndex(pMat, aiTextureType_DISPLACEMENT));

        // additional channel mapping
        if (pMat->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &strParam) == AI_SUCCESS)
        {
            material.settings.roughnessMapChannel = eTextureChannel::Green;
            material.settings.metallicMapChannel  = eTextureChannel::Blue;
        }

        // done
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

    if (m_options.bLoadStaticMesh)
    {
        flags |= aiProcess_Debone;
    }

    if (m_options.bDropExistingNormals)
    {
        flags |= aiProcess_DropNormals;
    }

    if (m_options.bLimitBondWeights)
    {
        flags |= aiProcess_LimitBoneWeights;
    }

    if (m_options.bLoadStaticMesh)
    {
        flags |= aiProcess_Debone;
    }

    Assimp::Importer importer = {};
    if (m_options.bLoadStaticMesh)
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
        aiMesh**    ppMeshes  = pAiScene->mMeshes;
        const Int32 numMeshes = NarrowCast<Int32>(pAiScene->mNumMeshes);
        m_meshes              = CreateImportedMeshes_(ppMeshes, numMeshes);
    }

    // texture load
    if (pAiScene->HasTextures())
    {
        aiTexture** pTextures   = pAiScene->mTextures;
        const Int32 numTextures = NarrowCast<Int32>(pAiScene->mNumTextures);
        m_textures              = CreateImportedTextures_(pTextures, numTextures);
    }

    // material load
    if (pAiScene->HasMaterials())
    {
        aiMaterial** pMats   = pAiScene->mMaterials;
        const Int32  numMats = NarrowCast<Int32>(pAiScene->mNumMaterials);
        m_materials          = CreateImportedMaterials_(pMats, numMats);
    }

    // build tree
    if (pAiScene->mRootNode)
    {
        m_rootNode = CreateImportedModelNode_Recursive(pAiScene->mRootNode, pAiScene);
    }

    return true;
}

void AssimpImporter::GenerateTangentFrame(
    const bool _bDropExistTangents)
{
    m_options.bGenerateTangentFrame     = true;
    m_options.bDropExistingTangentFrame = _bDropExistTangents;
}

void AssimpImporter::GenerateNormals(
    const bool _bDropExistNormals)
{
    m_options.bGenerateNormals     = true;
    m_options.bDropExistingNormals = _bDropExistNormals;
}

void AssimpImporter::LimitBoneWeight()
{
    m_options.bLimitBondWeights = true;
}

void AssimpImporter::LoadOptimizationForStaticMesh()
{
    m_options.bLoadStaticMesh = true;
}

void AssimpImporter::SetOptions(
    const AssimpImportOptions& _options)
{
    m_options = _options;
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

Ref<Model> AssimpImporter::CreateModel(
    const VertexLayout& _vl) const
{
    JAM_ASSERT(!m_meshes.empty(), "AssimpImporter::CreateModel() - No meshes to create model");
    JAM_ASSERT(!m_materials.empty(), "AssimpImporter::CreateModel() - No materials to create model");
    JAM_ASSERT(!m_textures.empty(), "AssimpImporter::CreateModel() - No textures to create model");

    // 데이터 -> 리소스 변환
    const Vector<Ref<Mesh>>         meshes    = CreateMeshes_(_vl);
    const Vector<Ref<ImageTexture>> textures  = CreateTextures_();
    const Vector<Ref<Material>>     materials = CreateMaterials_(textures);

    // 모델 생성
    Ref<Model> pModel = MakeRef<Model>();
    pModel->LoadMemory(m_rootNode, meshes, materials);
    return pModel;
}

Vector<Ref<Mesh>> AssimpImporter::CreateMeshes_(
    const VertexLayout& _vl) const
{
    JAM_ASSERT(!m_meshes.empty(), "AssimpImporter::CreateModel() - No meshes to create model");

    // vertex layout에 존재하는 속성 확인
    const bool bHasPositions  = _vl.Has(eVertexAttribute::Position);
    const bool bHasNormals    = _vl.Has(eVertexAttribute::Normal);
    const bool bHasTangents   = _vl.Has(eVertexAttribute::Tangent);
    const bool bHasBitangents = _vl.Has(eVertexAttribute::Bitangent);
    const bool bHasColor0     = _vl.Has(eVertexAttribute::Color0);
    const bool bHasTexCoord0  = _vl.Has(eVertexAttribute::TexCoord0);
    const bool bHasColor1     = _vl.Has(eVertexAttribute::Color1);
    const bool bHasTexCoord1  = _vl.Has(eVertexAttribute::TexCoord1);
    const bool bHasColor2     = _vl.Has(eVertexAttribute::Color2);
    const bool bHasTexCoord2  = _vl.Has(eVertexAttribute::TexCoord2);
    const bool bHasColor3     = _vl.Has(eVertexAttribute::Color3);
    const bool bHasTexCoord3  = _vl.Has(eVertexAttribute::TexCoord3);
    // const bool bHasBlendIndex  = _vl.Has(eVertexAttribute::BlendIndex);
    // const bool bHasBlendWeight = _vl.Has(eVertexAttribute::BlendWeight);

    // 첫 번째 메쉬를 기준으로 어떤 속성들이 임포트 되었는지 확인
    const ImportedMesh& firstMesh            = m_meshes[0];
    const bool          bIsPositionImported  = !firstMesh.positions.empty();
    const bool          bIsNormalImported    = !firstMesh.normals.empty();
    const bool          bIsTangentImported   = !firstMesh.tangents.empty();
    const bool          bIsBitangentImported = !firstMesh.bitangents.empty();
    const bool          bIsColor0Imported    = !firstMesh.colors[0].empty();
    const bool          bIsTexCoord0Imported = !firstMesh.texCoords[0].empty();
    const bool          bIsColor1Imported    = !firstMesh.colors[1].empty();
    const bool          bIsTexCoord1Imported = !firstMesh.texCoords[1].empty();
    const bool          bIsColor2Imported    = !firstMesh.colors[2].empty();
    const bool          bIsTexCoord2Imported = !firstMesh.texCoords[2].empty();
    const bool          bIsColor3Imported    = !firstMesh.colors[3].empty();
    const bool          bIsTexCoord3Imported = !firstMesh.texCoords[3].empty();
    // const bool          bIsBlendIndexImported = !firstMesh.blendIndices.empty();
    // const bool          bIsBlendWeightImported = !firstMesh.blendWeights.empty();

    // 메쉬별로 처리
    Vector<Ref<Mesh>> meshes = {};
    meshes.reserve(m_meshes.size());

    // 메쉬별로 버텍스 버퍼 작성
    for (const ImportedMesh& impMesh: m_meshes)
    {
        const Int32 stride    = _vl.GetStride();
        const Int32 numVerts  = impMesh.positions.size();
        const Int32 byteWidth = stride * numVerts;

        // 버텍스 버퍼 할당
        Vector<Byte> vertData = {};
        vertData.resize(byteWidth, 0);   // resize

        // vertex data 작성
        VertexWriter writer { vertData, _vl };
        for (Int32 i = 0; i < numVerts; ++i)
        {
            if (bHasPositions && bIsPositionImported)
            {
                writer.WriteAt(i, eVertexAttribute::Position, impMesh.positions[i]);
            }
            if (bHasNormals && bIsNormalImported)
            {
                writer.WriteAt(i, eVertexAttribute::Normal, impMesh.normals[i]);
            }
            if (bHasTangents && bIsTangentImported)
            {
                writer.WriteAt(i, eVertexAttribute::Tangent, impMesh.tangents[i]);
            }
            if (bHasBitangents && bIsBitangentImported)
            {
                writer.WriteAt(i, eVertexAttribute::Bitangent, impMesh.bitangents[i]);
            }
            if (bHasColor0 && bIsColor0Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color0, impMesh.colors[0][i]);
            }
            if (bHasTexCoord0 && bIsTexCoord0Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord0, impMesh.texCoords[0][i]);
            }
            if (bHasColor1 && bIsColor1Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color1, impMesh.colors[1][i]);
            }
            if (bHasTexCoord1 && bIsTexCoord1Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord1, impMesh.texCoords[1][i]);
            }
            if (bHasColor2 && bIsColor2Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color2, impMesh.colors[2][i]);
            }
            if (bHasTexCoord2 && bIsTexCoord2Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord2, impMesh.texCoords[2][i]);
            }
            if (bHasColor3 && bIsColor3Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color3, impMesh.colors[3][i]);
            }
            if (bHasTexCoord3 && bIsTexCoord3Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord3, impMesh.texCoords[3][i]);
            }
        }

        // 메쉬 생성
        Ref<Mesh> pMesh = MakeRef<Mesh>();
        pMesh->LoadMemory(vertData, _vl, impMesh.idxData, impMesh.bUseU32);
        pMesh->SetName(impMesh.name);
        meshes.emplace_back(pMesh);
    }

    return meshes;
}

Vector<Ref<ImageTexture>> AssimpImporter::CreateTextures_() const
{
    Vector<Ref<ImageTexture>> textures = {};
    textures.reserve(m_textures.size());

    Int32 i = 0;
    for (const ImportedTexture& impTex: m_textures)
    {
        TextureHandle texh = k_invalidHandle;
        if (impTex.bIsCompressed)
        {
            // 타입에 맞게 import 필요
            ImageImporter importer = {};
            if (!importer.ImportMemory(impTex.pixels, impTex.format))
            {
                JAM_ASSERT(0, "AssimpImporter::CreateTextures_() - Failed to import compressed texture in memory");
                textures.emplace_back(nullptr);   // placeholder for failed texture
            }

            // texture 생성
            texh = Graphics::CreateTexture(importer.GetScratchImage());
        }
        else
        {
            // 비압축된 경우, 이미지 포맷은 BGRA8_UNorm 고정
            const CMemView mems[] = { impTex.pixels };
            texh                  = Graphics::CreateTexture2D(impTex.width, impTex.height, eTextureFormat::BGRA8_UNorm, 1, 0, mems);
        }

        // 임시 이름
        const StringView name = GlobalBuffer::FormatTmp("texture_{}", i++);

        // texture 생성
        Ref<ImageTexture> pTexture = MakeRef<ImageTexture>();
        pTexture->LoadHandle(texh);
        pTexture->SetName(name);

        // 삽입
        textures.emplace_back(pTexture);
    }

    return textures;
}

Vector<Ref<Material>> AssimpImporter::CreateMaterials_(
    const Vector<Ref<ImageTexture>>& _textures) const
{
    Vector<Ref<Material>> materials = {};
    materials.reserve(m_materials.size());

    for (const ImportedMaterial& impMat: m_materials)
    {
        // 실제 텍스처와 연결
        MaterialTextures textures = {};

        // 텍스처 설정
        for (eMaterialTextureType type: eMaterialTextureType())
        {
            const Int32 texIdx = impMat.textures.Get(type);
            if (texIdx != k_invalidImportedIdx)
            {
                JAM_ASSERT(texIdx >= 0 && texIdx < NarrowCast<Int32>(_textures.size()), "AssimpImporter::CreateMaterials_() - Texture index out of range");
                textures.Set(type, _textures[texIdx]);
            }
        }

        // 머테리얼 생성
        Ref<Material> pMaterial = MakeRef<Material>();
        pMaterial->LoadMemory(impMat.params, textures, impMat.settings);
        pMaterial->SetName(impMat.name);

        // 삽입
        materials.emplace_back(pMaterial);
    }

    return materials;
}

void AssimpImporter::CleanUp_()
{
    m_rootNode = {};
    m_meshes.clear();
    m_materials.clear();
    m_textures.clear();
}

}   // namespace jam