#include "pch.h"

#include "AssimpImporter.h"

#include "MeshFactory.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "GeometryUtilities.h"
#include "Graphics.h"
#include "ImageBase.h"
#include "ImageImporter.h"
#include "Mesh.h"
#include "Model.h"

namespace Jam
{

namespace
{

    [[nodiscard]] Int32 ParseAsTextureIndexOrInvalid(   // << return k_invalidImportedIdx if failed
        const aiMaterial*   _pMat,
        const aiTextureType _type)
    {
        // 텍스처 이름
        aiString name = {};
        if (!_pMat->GetTexture(_type, 0, &name) == AI_SUCCESS)
        {
            return k_invalidImportedIdx;
        }

        // 이름 파싱 (이상적인 이름은 *{index} 형태)
        StringView nameStr = { name.C_Str(), name.length };
        if (!nameStr.StartsWith('*'))
        {
            return k_invalidImportedIdx;
        }

        // 인덱스 변환
        nameStr                   = nameStr.SubView(1);
        const Optional<Int32> idx = nameStr.ParseAs<Int32>();
        return idx ? idx.GetValue() : k_invalidImportedIdx;
    }

    [[nodiscard]] Vec2 Vec2From(
        const aiVector3D _vec)
    {
        return { _vec.x, _vec.y };
    }

    [[nodiscard]] Vec3 Vec3From(
        const aiVector3D _vec)
    {
        return { _vec.x, _vec.y, _vec.z };
    }

    [[nodiscard]] Vec4 Vec4From(
        const aiColor4D _color)
    {
        return { _color.r, _color.g, _color.b, _color.a };
    }

}   // namespace

Expect<AssimpImporter> AssimpImporter::ImportFromFile(
    const Path&                  _path,
    const AssimpImporterOptions& _options)
{
    AssimpImporter importer = {};
    if (const Error err = importer.ImportFromFile_(_path, _options))
    {
        return err;
    }

    return importer;
}

const ImportedModelNode& AssimpImporter::GetRootNode() const
{
    return m_root;
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

Expect<Ref<Model>> AssimpImporter::ConvertToModel(
    const VertexLayout& _vl) const
{
    JAM_ASSERT(!m_meshes.empty(), "AssimpImporter::ConvertToModel() - No meshes to create model");
    JAM_ASSERT(!m_materials.empty(), "AssimpImporter::ConvertToModel() - No materials to create model");
    JAM_ASSERT(!m_textures.empty(), "AssimpImporter::ConvertToModel() - No texes to create model");

    // 데이터 -> 리소스 변환
    const Vector<Ref<Mesh>>            meshes   = CreateMeshes_(_vl);
    const Expect<Vector<Ref<Texture>>> textures = CreateTextures_();
    if (!textures)
    {
        return textures.GetError();
    }
    const Vector<Ref<Material>> materials = CreateMaterials_(textures.GetValue());

    // 모델 생성
    Ref<Model> pModel = MakeRef<Model>();
    pModel->Load(m_root, meshes, materials);
    return pModel;
}

Error AssimpImporter::ImportFromFile_(
    const Path&                  _path,
    const AssimpImporterOptions& _options)
{
    UInt32 flags = aiProcess_JoinIdenticalVertices
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

    if (_options.bLoadStaticMesh)
    {
        flags |= aiProcess_Debone;
    }

    if (_options.bDropExistingNormals)
    {
        flags |= aiProcess_DropNormals;
    }

    if (_options.bLimitBondWeights)
    {
        flags |= aiProcess_LimitBoneWeights;
    }

    Assimp::Importer importer = {};
    if (_options.bLoadStaticMesh)
    {
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS);
    }
    else
    {
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS | aiComponent_BONEWEIGHTS | aiComponent_ANIMATIONS);
    }

    // remove line & point primitives
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);

    // import
    const aiScene* pAiScene = importer.ReadFile(_path, flags);
    if (!pAiScene)
    {
        JAM_LOG.Error("failed to load file: {}", importer.GetErrorString());
        return Error::ImportFailed;
    }

    ImportMeshes_(pAiScene, _options);
    if (const Error err = ImportTextures_(pAiScene))
    {
        return err;
    }
    ImportMaterials_(pAiScene);
    ImportModelNode_(pAiScene);
    return Error::None;
}

Vector<Ref<Mesh>> AssimpImporter::CreateMeshes_(
    const VertexLayout& _vl) const
{
    JAM_ASSERT(!m_meshes.empty(), "AssimpImporter::ConvertToModel() - No meshes to create model");

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
    for (const ImportedMesh& mesh: m_meshes)
    {
        const Int32 stride    = _vl.GetStride();
        const Int32 numVerts  = NumericCast<Int32>(mesh.positions.size());
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
                writer.WriteAt(i, eVertexAttribute::Position, mesh.positions[i]);
            }
            if (bHasNormals && bIsNormalImported)
            {
                writer.WriteAt(i, eVertexAttribute::Normal, mesh.normals[i]);
            }
            if (bHasTangents && bIsTangentImported)
            {
                writer.WriteAt(i, eVertexAttribute::Tangent, mesh.tangents[i]);
            }
            if (bHasBitangents && bIsBitangentImported)
            {
                writer.WriteAt(i, eVertexAttribute::Bitangent, mesh.bitangents[i]);
            }
            if (bHasColor0 && bIsColor0Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color0, mesh.colors[0][i]);
            }
            if (bHasTexCoord0 && bIsTexCoord0Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord0, mesh.texCoords[0][i]);
            }
            if (bHasColor1 && bIsColor1Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color1, mesh.colors[1][i]);
            }
            if (bHasTexCoord1 && bIsTexCoord1Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord1, mesh.texCoords[1][i]);
            }
            if (bHasColor2 && bIsColor2Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color2, mesh.colors[2][i]);
            }
            if (bHasTexCoord2 && bIsTexCoord2Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord2, mesh.texCoords[2][i]);
            }
            if (bHasColor3 && bIsColor3Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color3, mesh.colors[3][i]);
            }
            if (bHasTexCoord3 && bIsTexCoord3Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord3, mesh.texCoords[3][i]);
            }
        }

        // 메쉬 생성
        Ref<Mesh> pMesh = Mesh::Create(vertData, _vl, mesh.idxData, mesh.bUseU32);
        pMesh->SetName(mesh.name);
        meshes.emplace_back(pMesh);
    }

    return meshes;
}

Expect<Vector<Ref<Texture>>> AssimpImporter::CreateTextures_() const
{
    Vector<Ref<Texture>> texs = {};
    texs.reserve(m_textures.size());

    for (const ImportedTexture& impTex: m_textures)
    {
        TextureHandle texh = k_invalidHandle;
        if (impTex.bIsCompressed)
        {
            // 타입에 맞게 import 필요
            Expect<ImageImporter> importer = ImageImporter::ImportFromMemory(impTex.pixels, impTex.format);
            if (importer)
            {
                return importer.GetError();
            }

            // tex 생성
            texh = JAM_GFX.CreateTexture(importer->GetImageContainer());
        }
        else
        {
            // 비압축된 경우, 이미지 포맷은 BGRA8_UNorm 고정
            const CMemoryBlock mems[] = { impTex.pixels };
            texh                      = JAM_GFX.CreateTexture2D(impTex.width, impTex.height, eTextureFormat::BGRA8_UNorm, 1, eTextureCreateFlags::None, mems);
        }

        // tex 생성
        Ref<Texture> pTex = Texture::Create(texh);
        pTex->SetName(impTex.name);
        texs.emplace_back(pTex);
    }

    return texs;
}

Vector<Ref<Material>> AssimpImporter::CreateMaterials_(
    const Vector<Ref<Texture>>& _texes) const
{
    Vector<Ref<Material>> mats = {};
    mats.reserve(m_materials.size());

    for (const ImportedMaterial& impMat: m_materials)
    {
        // 실제 텍스처와 연결
        MaterialTextures texes = {};

        // 텍스처 설정
        for (eMaterialTextureType type: EnumRanges<eMaterialTextureType>())
        {
            if (impMat.texes.Has(type))
            {
                const Int32 texIdx = impMat.texes.Get(type);
                texes.Set(type, _texes[texIdx]);
            }
        }

        // 머테리얼 생성
        Ref<Material> pMat = Material::Create(impMat.params, texes, impMat.settings);
        pMat->SetName(impMat.name);
        mats.emplace_back(pMat);
    }

    return mats;
}

void AssimpImporter::ImportMeshes_(
    const aiScene*               _pScene,
    const AssimpImporterOptions& _options)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportMeshesFrom_() - _pScene is nullptr");
    JAM_ASSERT(m_meshes.empty(), "AssimpImporter::ImportMeshesFrom_() - m_meshes is not empty");

    if (_pScene->mNumMeshes == 0)
    {
        return;
    }

    const Int32 numMeshes = NumericCast<Int32>(_pScene->mNumMeshes);
    m_meshes.reserve(numMeshes);

    // import mesh
    for (Int32 i = 0; i < numMeshes; ++i)
    {
        ImportedMesh& mesh     = m_meshes.emplace_back();
        const aiMesh* pMesh    = _pScene->mMeshes[i];
        const Int32   numVerts = NumericCast<Int32>(pMesh->mNumVertices);

        // 이름
        {
            const aiString& aiName = pMesh->mName;

            ScratchBuffer tmp;
            tmp.Format("{}_{}", aiName.Empty() ? "mesh" : aiName.C_Str(), i);
            mesh.name = tmp.ToString();
        }

        // 인덱스
        {
            // 인텍스 포맷
            mesh.bUseU32 = NeedU32Index(numVerts);

            // 인덱스 버퍼 할당
            const Int32 numFaces  = NumericCast<Int32>(pMesh->mNumFaces);
            const Int32 numIdxes  = numFaces * 3;
            const Int32 byteWidth = CalcIndicesByteWidth(numIdxes, mesh.bUseU32);
            mesh.idxData.resize(byteWidth);

            // 인덱스 작성
            IndexWriter writer { mesh.idxData, mesh.bUseU32 };
            for (Int32 f = 0; f < numFaces; ++f)
            {
                const aiFace& aiFace = pMesh->mFaces[f];
                JAM_ASSERT(aiFace.mNumIndices == 3, "AssimpImporter::ImportImage() - Face is not a triangle");
                for (Int32 p = 0; p < 3; ++p)
                {
                    writer.WriteAt(f * 3 + p, aiFace.mIndices[p]);
                }
            }
        }

        // positions
        if (pMesh->HasPositions())
        {
            mesh.positions.reserve(numVerts);
            for (Int32 v = 0; v < numVerts; ++v)
            {
                const Vec3 pos = Vec3From(pMesh->mVertices[v]);
                mesh.positions.emplace_back(pos);
            }
        }

        // tex coordinates (up to 4)
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

        // colors (up to 4)
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

        // normals
        // _options.bGenerateNormals 가 true 여도. mesh에 normal이 있으면 그것을 우선 사용한다.
        if (pMesh->HasNormals() && !_options.bDropExistingNormals)
        {
            mesh.normals.reserve(numVerts);
            for (Int32 v = 0; v < numVerts; ++v)
            {
                const Vec3 normal = Vec3From(pMesh->mNormals[v]);
                mesh.normals.emplace_back(normal);
            }
        }
        else if (_options.bGenerateNormals)   // 내장 normals가 없거나, 드롭된 경우
        {
            mesh.normals = ComputeNormals(mesh.positions, mesh.idxData, mesh.bUseU32);
        }

        // tangents & bitangents
        if (pMesh->HasTangentsAndBitangents() && !_options.bDropExistingTangentFrame)
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
        else if (_options.bGenerateTangentFrame)   // 내장 tangent & bitangent가 없는 경우, 드롭된 경우
        {
            const TangentFrame tangentFrame = ComputeTangentFrame(mesh.positions, mesh.normals, mesh.texCoords[0], mesh.idxData, mesh.bUseU32);
            mesh.tangents                   = std::move(tangentFrame.tangents);
            mesh.bitangents                 = std::move(tangentFrame.bitangents);
        }
    }
}

Error AssimpImporter::ImportTextures_(
    const aiScene* _pScene)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportTexturesFrom_() - _pTexture is nullptr");
    JAM_ASSERT(m_textures.empty(), "AssimpImporter::ImportTexturesFrom_() - m_textures is not empty");

    if (!_pScene->HasTextures())
    {
        return Error::None;
    }

    const Int32 numTextures = NumericCast<Int32>(_pScene->mNumTextures);
    m_textures.reserve(numTextures);

    // 텍스처 순회
    for (Int32 i = 0; i < numTextures; ++i)
    {
        ImportedTexture& texture       = m_textures.emplace_back();
        const aiTexture* pTexture      = _pScene->mTextures[i];
        const bool       bIsCompressed = (pTexture->mHeight == 0);   // 압축된 텍스처는 mHeight가 0

        // 기본 정보
        texture.width         = NumericCast<Int32>(pTexture->mWidth);
        texture.height        = NumericCast<Int32>(pTexture->mHeight);
        texture.bIsCompressed = bIsCompressed;

        // 이름 생성
        {
            const aiString& filename = pTexture->mFilename;

            ScratchBuffer tmp;
            tmp.Format("{}_{}", filename.Empty() ? "texture" : filename.C_Str(), i);
            texture.name = tmp.ToString();
        }

        // 픽셀 데이터
        if (bIsCompressed)
        {
            texture.pixels = { pTexture->pcData, NumericCast<Int32>(pTexture->mWidth) };

            // 압축된 경우, format hint에 따라 포맷 결정
            const StringView             hint   = pTexture->achFormatHint;
            const Optional<eImageFormat> format = ParseAsImageFormat(hint);
            if (!format)
            {
                JAM_LOG.Error("AssimpImporter::ImportTextures_() - Failed to parse image format from hint: {}", hint);
                return Error::ImportFailed;
            }

            texture.format = format.GetValue();
        }
        else
        {
            // 비압축된 경우, RGBA8 포맷 고정
            const Int32 size = NumericCast<Int32>(texture.width * texture.height * sizeof(aiTexel));
            texture.pixels   = { pTexture->pcData, size };
        }
    }

    return Error::None;
}

void AssimpImporter::ImportMaterials_(
    const aiScene* _pScene)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportMaterialsFrom_() - _pScene is nullptr");
    JAM_ASSERT(m_materials.empty(), "AssimpImporter::ImportMaterialsFrom_() - m_materials is not empty");

    if (!_pScene->HasMaterials())
    {
        return;
    }

    const Int32 numMats = NumericCast<Int32>(_pScene->mNumMaterials);
    m_materials.reserve(numMats);

    // 머테리얼 순회
    for (Int32 i = 0; i < numMats; ++i)
    {
        ImportedMaterial& material = m_materials.emplace_back();
        const aiMaterial* pMat     = _pScene->mMaterials[i];

        // out params receiver
        aiString  strParam = {};
        aiColor3D colParam = {};
        float     fParam   = 0.f;
        int       iParam   = 0;

        // Name
        pMat->Get(AI_MATKEY_NAME, strParam);

        ScratchBuffer tmp;
        tmp.Format("{}_{}", strParam.Empty() ? "material" : strParam.C_Str(), i);
        material.name = tmp.ToString();

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

        // Base value / Albedo (PBR)
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
        material.texes.Set(eMaterialTextureType::Albedo, ParseAsTextureIndexOrInvalid(pMat, aiTextureType_DIFFUSE));
        material.texes.Set(eMaterialTextureType::Ao, ParseAsTextureIndexOrInvalid(pMat, aiTextureType_AMBIENT_OCCLUSION));
        material.texes.Set(eMaterialTextureType::Normal, ParseAsTextureIndexOrInvalid(pMat, aiTextureType_NORMALS));
        material.texes.Set(eMaterialTextureType::Metallic, ParseAsTextureIndexOrInvalid(pMat, aiTextureType_METALNESS));
        material.texes.Set(eMaterialTextureType::Roughness, ParseAsTextureIndexOrInvalid(pMat, aiTextureType_DIFFUSE_ROUGHNESS));
        material.texes.Set(eMaterialTextureType::Light, ParseAsTextureIndexOrInvalid(pMat, aiTextureType_LIGHTMAP));
        material.texes.Set(eMaterialTextureType::Emissive, ParseAsTextureIndexOrInvalid(pMat, aiTextureType_EMISSIVE));
        material.texes.Set(eMaterialTextureType::VertexDisplacement, ParseAsTextureIndexOrInvalid(pMat, aiTextureType_DISPLACEMENT));

        // additional channel mapping
        if (pMat->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &strParam) == AI_SUCCESS)
        {
            material.settings.roughnessMapChannel = eTextureChannel::Green;
            material.settings.metallicMapChannel  = eTextureChannel::Blue;
        }
    }
}

void AssimpImporter::ImportModelNode_(
    const aiScene* _pScene)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportModelNode_() - _pScene is nullptr");
    m_root = CreateImportModelNode_Recursive_(_pScene, _pScene->mRootNode);
}

ImportedModelNode AssimpImporter::CreateImportModelNode_Recursive_(
    const aiScene* _pScene,
    const aiNode*  _pNode)
{
    ImportedModelNode node = {};

    // 이름
    node.name = { _pNode->mName.C_Str(), _pNode->mName.length };

    // 행렬
    {
        const aiMatrix4x4& aiMtx = _pNode->mTransformation;

        Mtx4 mtx = {};
        mtx._11 = aiMtx.a1, mtx._12 = aiMtx.a2, mtx._13 = aiMtx.a3, mtx._14 = aiMtx.a4;
        mtx._21 = aiMtx.b1, mtx._22 = aiMtx.b2, mtx._23 = aiMtx.b3, mtx._24 = aiMtx.b4;
        mtx._31 = aiMtx.c1, mtx._32 = aiMtx.c2, mtx._33 = aiMtx.c3, mtx._34 = aiMtx.c4;
        mtx._41 = aiMtx.d1, mtx._42 = aiMtx.d2, mtx._43 = aiMtx.d3, mtx._44 = aiMtx.d4;
        node.mtx = mtx;
    }

    // 메쉬와 메쉬의 머테리얼 인덱스
    const Int32 numMeshes = NumericCast<Int32>(_pNode->mNumMeshes);
    if (numMeshes > 0)
    {
        node.meshIdxes.reserve(numMeshes);
        node.matIdxes.reserve(numMeshes);

        // 메쉬는 반드시 하나의 머테리얼과 연관되어있음
        for (Int32 i = 0; i < numMeshes; ++i)
        {
            const Int32   meshIdx = NumericCast<Int32>(_pNode->mMeshes[i]);
            const aiMesh* pMesh   = _pScene->mMeshes[meshIdx];
            const Int32   matIdx  = NumericCast<Int32>(pMesh->mMaterialIndex);

            node.meshIdxes.emplace_back(meshIdx);
            node.matIdxes.emplace_back(matIdx);
        }
    }

    // 하위 자식 노드들
    const Int32 numChildren = NumericCast<Int32>(_pNode->mNumChildren);
    node.children.reserve(numChildren);
    for (Int32 i = 0; i < numChildren; ++i)
    {
        node.children.emplace_back(CreateImportModelNode_Recursive_(_pScene, _pNode->mChildren[i]));
    }

    return node;
}

}   // namespace Jam