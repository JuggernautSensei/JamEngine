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

    [[nodiscard]] Int32 ParseAsTextureIndexOrInvalid_(   // << return k_invalidImportedIdx if failed
        const aiMaterial*   _pMaterial,
        const aiTextureType _type)
    {
        // 텍스처 이름
        aiString name = {};
        if (_pMaterial->GetTexture(_type, 0, &name) != AI_SUCCESS)
        {
            return k_invalidImportedTextureIdx;
        }

        // 이름 파싱 (이상적인 이름은 *{index} 형태)
        StringView nameStr = { name.C_Str(), name.length };
        if (!nameStr.StartsWith('*'))
        {
            return k_invalidImportedTextureIdx;
        }

        // 인덱스 변환
        nameStr                   = nameStr.SubView(1);
        const Optional<Int32> idx = nameStr.ParseAs<Int32>();
        return idx ? idx.GetValue() : k_invalidImportedTextureIdx;
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

Expect<AssimpImporter> AssimpImporter::ImportFromMemory(
    const MemoryView          _data,
    const AssimpImporterInfo& _info)
{
    AssimpImporter importer = {};
    if (const Error err = importer.ImportFromMemory_(_data, _info))
    {
        return err;
    }
    return importer;
}

Expect<AssimpImporter> AssimpImporter::ImportFromFile(
    const StringView          _path,
    const AssimpImporterInfo& _info)
{
    AssimpImporter importer = {};
    if (const Error err = importer.ImportFromFile_(_path, _info))
    {
        return err;
    }
    return importer;
}

const ImportedModelNode& AssimpImporter::GetRootNode() const
{
    return m_importedRoot;
}

const Vector<ImportedMesh>& AssimpImporter::GetMeshes() const
{
    return m_importedMeshes;
}

const Vector<ImportedMaterial>& AssimpImporter::GetMaterials() const
{
    return m_importedMaterials;
}

const Vector<ImportedTexture>& AssimpImporter::GetTextures() const
{
    return m_importedTextures;
}

Expect<Ref<Model>> AssimpImporter::ExtractToModel(
    const VertexLayout& _vl)
{
    JAM_ASSERT(!m_bExtracted, "AssimpImporter::ExtractToModel() - Cannot extract more than once");

    // texture
    const Expect<Vector<Ref<Texture>>> textures = ConvertToTextures_();
    if (!textures)
    {
        return textures.GetError();
    }

    ModelDesc desc = {};
    desc.meshes    = ConvertToMeshes_(_vl);
    desc.materials = ConvertToMaterials_(textures.GetValue());
    desc.root      = std::move(m_importedRoot);
    m_bExtracted   = true;
    return Model::Load(std::move(desc));
}

Error AssimpImporter::ImportFromMemory_(
    const MemoryView          _data,
    const AssimpImporterInfo& _options)
{
    Assimp::Importer importer = {};
    PreImport_(importer, _options);
    const aiScene* pScene = importer.ReadFileFromMemory(_data.GetPtr(), _data.GetByteWidth(), MakeImportFlags_(_options));
    if (!pScene)
    {
        JAM_LOG_ERROR("failed to load from memory: {}", importer.GetErrorString());
        return Error::ImportFailed;
    }

    return Import_(pScene, _options);
}

Error AssimpImporter::ImportFromFile_(
    const StringView          _path,
    const AssimpImporterInfo& _options)
{
    Assimp::Importer importer = {};
    PreImport_(importer, _options);
    const aiScene* pScene = importer.ReadFile(_path.GetPtr(), MakeImportFlags_(_options));
    if (!pScene)
    {
        JAM_LOG_ERROR("failed to load file: {}", importer.GetErrorString());
        return Error::ImportFailed;
    }

    return Import_(pScene, _options);
}

Error AssimpImporter::Import_(
    const aiScene*            _pScene,
    const AssimpImporterInfo& _options)
{
    JAM_ASSERT(_pScene, "AssimpImporter::Import_() - aiScene is null");
    ImportMeshes_(_pScene, _options);
    if (const Error err = ImportTextures_(_pScene))
    {
        return err;
    }
    ImportMaterials_(_pScene);
    ImportModelNode_(_pScene);
    return Error::None;
}

UInt32 AssimpImporter::MakeImportFlags_(
    const AssimpImporterInfo& _options) const
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

    return flags;
}

void AssimpImporter::PreImport_(
    Assimp::Importer&         _importer,
    const AssimpImporterInfo& _options) const
{
    if (_options.bLoadStaticMesh)
    {
        _importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS);
    }
    else
    {
        _importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS | aiComponent_BONEWEIGHTS | aiComponent_ANIMATIONS);
    }

    // remove line & point primitives
    _importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
}

Vector<Ref<Mesh>> AssimpImporter::ConvertToMeshes_(
    const VertexLayout& _vl) const
{
    JAM_ASSERT(!m_importedMeshes.empty(), "AssimpImporter::ExtractToModel() - No meshes to create model");

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
    const ImportedMesh& firstMesh            = m_importedMeshes[0];
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
    meshes.reserve(m_importedMeshes.size());

    // 메쉬별로 버텍스 버퍼 작성
    for (const ImportedMesh& importedMesh: m_importedMeshes)
    {
        const Int32 stride    = _vl.GetStride();
        const Int32 numVerts  = NumericCast<Int32>(importedMesh.positions.size());
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
                writer.WriteAt(i, eVertexAttribute::Position, importedMesh.positions[i]);
            }
            if (bHasNormals && bIsNormalImported)
            {
                writer.WriteAt(i, eVertexAttribute::Normal, importedMesh.normals[i]);
            }
            if (bHasTangents && bIsTangentImported)
            {
                writer.WriteAt(i, eVertexAttribute::Tangent, importedMesh.tangents[i]);
            }
            if (bHasBitangents && bIsBitangentImported)
            {
                writer.WriteAt(i, eVertexAttribute::Bitangent, importedMesh.bitangents[i]);
            }
            if (bHasColor0 && bIsColor0Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color0, importedMesh.colors[0][i]);
            }
            if (bHasTexCoord0 && bIsTexCoord0Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord0, importedMesh.texCoords[0][i]);
            }
            if (bHasColor1 && bIsColor1Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color1, importedMesh.colors[1][i]);
            }
            if (bHasTexCoord1 && bIsTexCoord1Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord1, importedMesh.texCoords[1][i]);
            }
            if (bHasColor2 && bIsColor2Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color2, importedMesh.colors[2][i]);
            }
            if (bHasTexCoord2 && bIsTexCoord2Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord2, importedMesh.texCoords[2][i]);
            }
            if (bHasColor3 && bIsColor3Imported)
            {
                writer.WriteAt(i, eVertexAttribute::Color3, importedMesh.colors[3][i]);
            }
            if (bHasTexCoord3 && bIsTexCoord3Imported)
            {
                writer.WriteAt(i, eVertexAttribute::TexCoord3, importedMesh.texCoords[3][i]);
            }
        }

        // 메쉬 생성
        Ref<Mesh> pMesh = Mesh::Load(vertData, _vl, importedMesh.idxData, importedMesh.bUseU32);
        pMesh->SetName(importedMesh.name);
        meshes.emplace_back(pMesh);
    }

    return meshes;
}

Expect<Vector<Ref<Texture>>> AssimpImporter::ConvertToTextures_() const
{
    Vector<Ref<Texture>> textures = {};
    textures.reserve(m_importedTextures.size());

    for (const ImportedTexture& importedTexture: m_importedTextures)
    {
        TextureHandle texh = k_nullHandle;
        if (importedTexture.bIsCompressed)
        {
            // 타입에 맞게 import 필요
            Expect<ImageImporter> importer = ImageImporter::ImportFromMemory(importedTexture.pixels, importedTexture.format);
            if (!importer)
            {
                return importer.GetError();
            }

            // texture 생성
            texh = GetGraphics().CreateTexture(importer->GetImageContainer());
        }
        else
        {
            // 비압축된 경우, 이미지 포맷은 BGRA8_UNorm 고정
            const MemoryView mems[] = { importedTexture.pixels };
            texh                    = GetGraphics().CreateTexture2D(importedTexture.width, importedTexture.height, eTextureFormat::BGRA8_UNorm, 1, eTextureCreateFlags::None, mems);
        }

        // texture 생성
        Ref<Texture> pTexture = Texture::Load(texh);
        pTexture->SetName(importedTexture.name);
        textures.emplace_back(pTexture);
    }

    return textures;
}

Vector<Ref<Material>> AssimpImporter::ConvertToMaterials_(
    const Vector<Ref<Texture>>& _textures) const
{
    Vector<Ref<Material>> materials = {};
    materials.reserve(m_importedMaterials.size());

    for (const ImportedMaterial& importedMaterial: m_importedMaterials)
    {
        MaterialDesc desc = {};
        desc.params       = importedMaterial.params;
        desc.settings     = importedMaterial.settings;

        // importedMaterial은 전역 textures의 인덱스를 가지고 있고, 그것을 이용하여 실제 텍스처를 매핑함
        for (eMaterialTexture type: RangesOf<eMaterialTexture>())
        {
            const Int32 idx = importedMaterial.textures[type];
            if (idx != k_invalidImportedTextureIdx)
            {
                desc.textures[type] = _textures[idx];
            }
        }

        // 머테리얼 생성
        Ref<Material> pMaterial = Material::Load(desc);
        pMaterial->SetName(importedMaterial.name);
        materials.emplace_back(pMaterial);
    }

    return materials;
}

void AssimpImporter::ImportMeshes_(
    const aiScene*            _pScene,
    const AssimpImporterInfo& _options)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportMeshesFrom_() - _pScene is nullptr");
    JAM_ASSERT(m_importedMeshes.empty(), "AssimpImporter::ImportMeshesFrom_() - m_meshes is not empty");

    if (_pScene->mNumMeshes == 0)
    {
        return;
    }

    const Int32 numMeshes = NumericCast<Int32>(_pScene->mNumMeshes);
    m_importedMeshes.reserve(numMeshes);

    // import mesh
    for (Int32 i = 0; i < numMeshes; ++i)
    {
        ImportedMesh& importedMesh = m_importedMeshes.emplace_back();
        const aiMesh* pMesh        = _pScene->mMeshes[i];
        const Int32   numVerts     = NumericCast<Int32>(pMesh->mNumVertices);

        // 이름
        {
            const aiString& aiName = pMesh->mName;
            importedMesh.name      = String::Formatted("{}_{}", aiName.Empty() ? "mesh" : aiName.C_Str(), i);
        }

        // 인덱스
        {
            // 인텍스 포맷
            importedMesh.bUseU32 = NeedU32Index(numVerts);

            // 인덱스 버퍼 할당
            const Int32 numFaces  = NumericCast<Int32>(pMesh->mNumFaces);
            const Int32 numIdxes  = numFaces * 3;
            const Int32 byteWidth = CalcIndicesByteWidth(numIdxes, importedMesh.bUseU32);
            importedMesh.idxData.resize(byteWidth);

            // 인덱스 작성
            IndexWriter writer { importedMesh.idxData, importedMesh.bUseU32 };
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
            importedMesh.positions.reserve(numVerts);
            for (Int32 v = 0; v < numVerts; ++v)
            {
                const Vec3 pos = Vec3From(pMesh->mVertices[v]);
                importedMesh.positions.emplace_back(pos);
            }
        }

        // texture coordinates (up to 4)
        for (Int32 j = 0; j < 4; ++j)
        {
            if (pMesh->HasTextureCoords(j))
            {
                Vector<Vec2>& texCoord = importedMesh.texCoords[j];
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
                Vector<Vec4>& color = importedMesh.colors[j];
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
            importedMesh.normals.reserve(numVerts);
            for (Int32 v = 0; v < numVerts; ++v)
            {
                const Vec3 normal = Vec3From(pMesh->mNormals[v]);
                importedMesh.normals.emplace_back(normal);
            }
        }
        else if (_options.bGenerateNormals)   // 내장 normals가 없거나, 드롭된 경우
        {
            importedMesh.normals = ComputeNormals(importedMesh.positions, importedMesh.idxData, importedMesh.bUseU32);
        }

        // tangents & bitangents
        if (pMesh->HasTangentsAndBitangents() && !_options.bDropExistingTangentFrame)
        {
            importedMesh.tangents.reserve(numVerts);
            importedMesh.bitangents.reserve(numVerts);
            for (Int32 v = 0; v < numVerts; ++v)
            {
                const Vec3 tangent   = Vec3From(pMesh->mTangents[v]);
                const Vec3 bitangent = Vec3From(pMesh->mBitangents[v]);
                importedMesh.tangents.emplace_back(tangent);
                importedMesh.bitangents.emplace_back(bitangent);
            }
        }
        else if (_options.bGenerateTangentFrame)   // 내장 tangent & bitangent가 없는 경우, 드롭된 경우
        {
            TangentFrame tangentFrame = ComputeTangentFrame(importedMesh.positions, importedMesh.normals, importedMesh.texCoords[0], importedMesh.idxData, importedMesh.bUseU32);
            importedMesh.tangents     = std::move(tangentFrame.tangents);
            importedMesh.bitangents   = std::move(tangentFrame.bitangents);
        }
    }
}

Error AssimpImporter::ImportTextures_(
    const aiScene* _pScene)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportTexturesFrom_() - _pTexture is nullptr");
    JAM_ASSERT(m_importedTextures.empty(), "AssimpImporter::ImportTexturesFrom_() - m_textures is not empty");

    if (!_pScene->HasTextures())
    {
        return Error::None;
    }

    const Int32 numTextures = NumericCast<Int32>(_pScene->mNumTextures);
    m_importedTextures.reserve(numTextures);

    // 텍스처 순회
    for (Int32 i = 0; i < numTextures; ++i)
    {
        ImportedTexture& importedTexture = m_importedTextures.emplace_back();
        const aiTexture* pTexture        = _pScene->mTextures[i];
        const bool       bIsCompressed   = (pTexture->mHeight == 0);   // 압축된 텍스처는 mHeight가 0

        // 기본 정보
        importedTexture.width         = NumericCast<Int32>(pTexture->mWidth);
        importedTexture.height        = NumericCast<Int32>(pTexture->mHeight);
        importedTexture.bIsCompressed = bIsCompressed;

        // 이름 생성
        {
            const aiString& filename = pTexture->mFilename;
            importedTexture.name     = String::Formatted("{}_{}", filename.Empty() ? "texture" : filename.C_Str(), i);
        }

        // 픽셀 데이터
        if (bIsCompressed)
        {
            importedTexture.pixels = { pTexture->pcData, NumericCast<Int32>(pTexture->mWidth) };

            // 압축된 경우, format hint에 따라 포맷 결정
            const StringView             hint   = pTexture->achFormatHint;
            const Optional<eImageFormat> format = ParseAsImageFormat(hint);
            if (!format)
            {
                JAM_LOG_ERROR("AssimpImporter::ImportTextures_() - Failed to parse image format from hint: {}", hint);
                return Error::ImportFailed;
            }

            importedTexture.format = format.GetValue();
        }
        else
        {
            // 비압축된 경우, RGBA8 포맷 고정
            const Int32 size       = NumericCast<Int32>(importedTexture.width * importedTexture.height * sizeof(aiTexel));
            importedTexture.pixels = { pTexture->pcData, size };
        }
    }

    return Error::None;
}

void AssimpImporter::ImportMaterials_(
    const aiScene* _pScene)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportMaterialsFrom_() - _pScene is nullptr");
    JAM_ASSERT(m_importedMaterials.empty(), "AssimpImporter::ImportMaterialsFrom_() - m_materials is not empty");

    if (!_pScene->HasMaterials())
    {
        return;
    }

    const Int32 numMats = NumericCast<Int32>(_pScene->mNumMaterials);
    m_importedMaterials.reserve(numMats);

    // 머테리얼 순회
    for (Int32 i = 0; i < numMats; ++i)
    {
        ImportedMaterial& importedMaterial = m_importedMaterials.emplace_back();
        const aiMaterial* pMaterial        = _pScene->mMaterials[i];

        // out params receiver
        aiString  strParam = {};
        aiColor3D colParam = {};
        float     fParam   = 0.f;
        int       iParam   = 0;

        // Name
        const bool bHasName   = pMaterial->Get(AI_MATKEY_NAME, strParam) == AI_SUCCESS;
        importedMaterial.name = String::Formatted("{}_{}", (!bHasName || strParam.Empty()) ? "material" : strParam.C_Str(), i);

        // Ambient
        if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, colParam) == AI_SUCCESS)
        {
            importedMaterial.params.ambient = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Diffuse
        if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, colParam) == AI_SUCCESS)
        {
            importedMaterial.params.diffuse = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Specular
        if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, colParam) == AI_SUCCESS)
        {
            importedMaterial.params.specular = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Shininess
        if (pMaterial->Get(AI_MATKEY_SHININESS, fParam) == AI_SUCCESS)
        {
            importedMaterial.params.shininess = fParam;
        }

        // Base value / Albedo (PBR)
        if (pMaterial->Get(AI_MATKEY_BASE_COLOR, colParam) == AI_SUCCESS)
        {
            importedMaterial.params.albedo = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // Roughness
        if (pMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, fParam) == AI_SUCCESS)
        {
            importedMaterial.params.roughness = fParam;
        }

        // Metallic
        if (pMaterial->Get(AI_MATKEY_METALLIC_FACTOR, fParam) == AI_SUCCESS)
        {
            importedMaterial.params.metallic = fParam;
        }

        // Two sided
        if (pMaterial->Get(AI_MATKEY_TWOSIDED, iParam) == AI_SUCCESS)
        {
            importedMaterial.params.bDoubleSided = (iParam != 0);
        }

        // Emissive
        if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, colParam) == AI_SUCCESS)
        {
            importedMaterial.params.emissive = Vec3 { colParam.r, colParam.g, colParam.b };
        }

        // emissive intensity
        if (pMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, fParam) == AI_SUCCESS)
        {
            importedMaterial.params.emissiveIntensity = fParam;
        }

        // Mapture parsing

        importedMaterial.textures[eMaterialTexture::Albedo]             = ParseAsTextureIndexOrInvalid_(pMaterial, aiTextureType_BASE_COLOR);
        importedMaterial.textures[eMaterialTexture::Ao]                 = ParseAsTextureIndexOrInvalid_(pMaterial, aiTextureType_AMBIENT_OCCLUSION);
        importedMaterial.textures[eMaterialTexture::Normal]             = ParseAsTextureIndexOrInvalid_(pMaterial, aiTextureType_NORMALS);
        importedMaterial.textures[eMaterialTexture::Metallic]           = ParseAsTextureIndexOrInvalid_(pMaterial, aiTextureType_METALNESS);
        importedMaterial.textures[eMaterialTexture::Roughness]          = ParseAsTextureIndexOrInvalid_(pMaterial, aiTextureType_DIFFUSE_ROUGHNESS);
        importedMaterial.textures[eMaterialTexture::Light]              = ParseAsTextureIndexOrInvalid_(pMaterial, aiTextureType_LIGHTMAP);
        importedMaterial.textures[eMaterialTexture::Emissive]           = ParseAsTextureIndexOrInvalid_(pMaterial, aiTextureType_EMISSIVE);
        importedMaterial.textures[eMaterialTexture::VertexDisplacement] = ParseAsTextureIndexOrInvalid_(pMaterial, aiTextureType_DISPLACEMENT);

        // additional channel mapping
        if (pMaterial->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &strParam) == AI_SUCCESS)
        {
            importedMaterial.settings.roughnessMapChannel = eTextureChannel::Green;
            importedMaterial.settings.metallicMapChannel  = eTextureChannel::Blue;
        }
    }
}

void AssimpImporter::ImportModelNode_(
    const aiScene* _pScene)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportModelNode_() - _pScene is nullptr");
    m_importedRoot = ImportModelNode_Recursive_(_pScene, _pScene->mRootNode);
}

ImportedModelNode AssimpImporter::ImportModelNode_Recursive_(
    const aiScene* _pScene,
    const aiNode*  _pNode)
{
    ImportedModelNode importedNode = {};

    // 이름
    importedNode.name = String { _pNode->mName.C_Str(), _pNode->mName.length };

    // 행렬
    {
        const aiMatrix4x4& aiMtx = _pNode->mTransformation;

        Mtx4 mtx = {};
        mtx._11 = aiMtx.a1, mtx._12 = aiMtx.a2, mtx._13 = aiMtx.a3, mtx._14 = aiMtx.a4;
        mtx._21 = aiMtx.b1, mtx._22 = aiMtx.b2, mtx._23 = aiMtx.b3, mtx._24 = aiMtx.b4;
        mtx._31 = aiMtx.c1, mtx._32 = aiMtx.c2, mtx._33 = aiMtx.c3, mtx._34 = aiMtx.c4;
        mtx._41 = aiMtx.d1, mtx._42 = aiMtx.d2, mtx._43 = aiMtx.d3, mtx._44 = aiMtx.d4;
        importedNode.mtx = mtx;
    }

    // 메쉬와 메쉬의 머테리얼 인덱스
    const Int32 numMeshes = NumericCast<Int32>(_pNode->mNumMeshes);
    if (numMeshes > 0)
    {
        importedNode.meshIdxes.reserve(numMeshes);
        importedNode.matIdxes.reserve(numMeshes);

        // 메쉬는 반드시 하나의 머테리얼과 연관되어있음
        for (Int32 i = 0; i < numMeshes; ++i)
        {
            const Int32   meshIdx = NumericCast<Int32>(_pNode->mMeshes[i]);
            const aiMesh* pMesh   = _pScene->mMeshes[meshIdx];
            const Int32   matIdx  = NumericCast<Int32>(pMesh->mMaterialIndex);

            importedNode.meshIdxes.emplace_back(meshIdx);
            importedNode.matIdxes.emplace_back(matIdx);
        }
    }

    // 하위 자식 노드들
    const Int32 numChildren = NumericCast<Int32>(_pNode->mNumChildren);
    importedNode.children.reserve(numChildren);
    for (Int32 i = 0; i < numChildren; ++i)
    {
        importedNode.children.emplace_back(ImportModelNode_Recursive_(_pScene, _pNode->mChildren[i]));
    }

    return importedNode;
}

}   // namespace Jam