#include "pch.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cmath>

#include "Mesh.h"
#include "Model.h"
#include "Texture.h"
#include "Profile.h"
#include "Graphics.h"
#include "ModelImporter.h"
#include "GeometryReadWriter.h"
#include "Image.h"

namespace Jam
{

namespace
{

    [[nodiscard]] Expect<Int32> ParseAsTextureIndex_(
        const aiScene*      _pScene,
        const aiMaterial*   _pMaterial,
        const aiTextureType _type)
    {
        aiString name = {};
        if (_pMaterial->GetTexture(_type, 0, &name) != AI_SUCCESS)
        {
            return Error::ImportFailed;
        }

        if (const aiTexture* texture = _pScene->GetEmbeddedTexture(name.C_Str()))
        {
            for (Int32 i = 0; i < static_cast<Int32>(_pScene->mNumTextures); ++i)
                if (_pScene->mTextures[i] == texture)
                    return i;
        }

        StringView str = { name.C_Str(), name.length };
        if (!str.StartsWith('*'))
        {
            return Error::ImportFailed;
        }

        // 인덱스 변환
        str                              = str.SubView(1);
        const Optional<Int32> indexOrNot = str.ParseAs<Int32>();
        return indexOrNot ? indexOrNot.GetValue() : Error::ParseError;
    }

    [[nodiscard]] Vec2 AsVec2(
        const aiVector3D _vec)
    {
        return { _vec.x, _vec.y };
    }

    [[nodiscard]] Vec3 AsVec3(
        const aiVector3D _vec)
    {
        return { _vec.x, _vec.y, _vec.z };
    }

    [[nodiscard]] Vec4 AsVec4(
        const aiColor4D _color)
    {
        return { _color.r, _color.g, _color.b, _color.a };
    }

    Mtx4 AsMtx4_(const aiMatrix4x4& _m)
    {
        // assimp 열 벡터, jam 행 벡터
        return { _m.a1, _m.b1, _m.c1, _m.d1, _m.a2, _m.b2, _m.c2, _m.d2, _m.a3, _m.b3, _m.c3, _m.d3, _m.a4, _m.b4, _m.c4, _m.d4 };
    }

}   // namespace

Expect<AssimpImporter> AssimpImporter::ImportFromMemory(
    const MemoryView                  _data,
    const Flags<eAssimpImporterFlags> _flags)
{
    AssimpImporter importer = {};
    if (const Error err = importer.ImportFromMemory_(_data, _flags))
    {
        return err;
    }
    return importer;
}

Expect<AssimpImporter> AssimpImporter::ImportFromFile(
    const StringView                  _path,
    const Flags<eAssimpImporterFlags> _flags)
{
    AssimpImporter importer = {};
    if (const Error err = importer.ImportFromFile_(_path, _flags))
    {
        return err;
    }
    return importer;
}

const ModelNode& AssimpImporter::GetRoot() const
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

const SkeletalAnimationData& AssimpImporter::GetAnimationData() const
{
    return m_animationData;
}

Expect<Ref<Model>> AssimpImporter::ConvertToModel(
    const VertexLayout&                     _attribLayoutOrEmpty,
    const Flags<eAssimpConvertToModelFlags> _flags)
{
    Ref<Model> pModel = Model::Load();

    // mesh
    if (_flags & eAssimpConvertToModelFlags::Mesh)
    {
        pModel->SetMeshes(ConvertToMeshes_(_attribLayoutOrEmpty));
    }

    // texture
    Vector<Ref<Texture>> textures = {};
    if (_flags & eAssimpConvertToModelFlags::Texture)
    {
        Expect<Vector<Ref<Texture>>> texturesOrNot = ConvertToTextures_();
        if (!texturesOrNot)
        {
            return texturesOrNot.GetError();
        }
        textures = texturesOrNot.Take();
    }

    // material
    if (_flags & eAssimpConvertToModelFlags::Material)
    {
        pModel->SetMaterials(ConvertToMaterials_(textures));
    }

    // root
    if (_flags & eAssimpConvertToModelFlags::NoInvalidate)
    {
        pModel->SetRootNode(m_importedRoot);
    }
    else
    {
        pModel->SetRootNode(std::move(m_importedRoot));
    }

    return pModel;
}

Error AssimpImporter::ImportFromMemory_(
    const MemoryView                   _data,
    const Flags<eAssimpImporterFlags>& _flags)
{
    if (_flags.IsEnabled(eAssimpImporterFlags::LoadAnimations) && _flags.IsEnabled(eAssimpImporterFlags::LoadStaticMesh))
        return Error::InvalidArgument;
    Assimp::Importer importer = {};
    PreImport_(importer, _flags);
    const aiScene* pScene = importer.ReadFileFromMemory(_data.GetPtr(), _data.GetByteWidth(), MakeImportFlags_(_flags));
    if (!pScene)
    {
        JAM_LOG_ERROR("failed to load from memory: {}", importer.GetErrorString());
        return Error::ImportFailed;
    }
    return Import_(pScene, _flags);
}

Error AssimpImporter::ImportFromFile_(
    const StringView                   _path,
    const Flags<eAssimpImporterFlags>& _flags)
{
    JAM_FUNCTION_PROFILE();
    if (_flags.IsEnabled(eAssimpImporterFlags::LoadAnimations) && _flags.IsEnabled(eAssimpImporterFlags::LoadStaticMesh))
        return Error::InvalidArgument;

    Assimp::Importer importer = {};
    PreImport_(importer, _flags);
    const aiScene* pScene = importer.ReadFile(_path.GetPtr(), MakeImportFlags_(_flags));
    if (!pScene)
    {
        JAM_LOG_ERROR("failed to load file: {}", importer.GetErrorString());
        return Error::ImportFailed;
    }
    return Import_(pScene, _flags);
}

Error AssimpImporter::Import_(
    const aiScene*                    _pScene,
    const Flags<eAssimpImporterFlags> _flags)
{
    JAM_ASSERT(_pScene, "AssimpImporter::Import_() - aiScene is null");
    JAM_FUNCTION_PROFILE();

    bool bAnyLoaded = false;
    if (_flags & eAssimpImporterFlags::LoadMeshes)
    {
        ImportMeshes_(_pScene);
        bAnyLoaded = true;
    }

    if (_flags & eAssimpImporterFlags::LoadTextures)
    {
        if (const Error err = ImportTextures_(_pScene))
        {
            return err;
        }
        bAnyLoaded = true;
    }

    if (_flags & eAssimpImporterFlags::LoadMaterials)
    {
        ImportMaterials_(_pScene);
        bAnyLoaded = true;
    }

    if (_flags.IsEnabled(eAssimpImporterFlags::LoadAnimations))
    {
        if (const Error err = ImportAnimations_(_pScene))
            return err;
        bAnyLoaded = true;
    }

    if (bAnyLoaded)
    {
        ImportModelNode_(_pScene);
    }

    return Error::None;
}

UInt32 AssimpImporter::MakeImportFlags_(
    const Flags<eAssimpImporterFlags>& _flags) const
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

    if (_flags.IsEnabled(eAssimpImporterFlags::LoadStaticMesh))
    {
        flags |= aiProcess_Debone;
    }

    if (_flags.IsEnabled(eAssimpImporterFlags::DropExistingNormals))
    {
        flags |= aiProcess_DropNormals;
    }

    if (_flags.IsEnabled(eAssimpImporterFlags::LimitBoneWeights) || _flags.IsEnabled(eAssimpImporterFlags::LoadAnimations))
    {
        flags |= aiProcess_LimitBoneWeights;
    }

    if (_flags.IsEnabled(eAssimpImporterFlags::DropExistingNormals))
    {
        flags |= aiProcess_DropNormals;
    }

    if (_flags.IsEnabled(eAssimpImporterFlags::GenerateSmoothNormals))
    {
        flags |= aiProcess_GenSmoothNormals;
    }
    else if (_flags.IsEnabled(eAssimpImporterFlags::GenerateNormals))
    {
        flags |= aiProcess_GenNormals;
    }

    if (_flags.IsEnabled(eAssimpImporterFlags::GenerateTangentFrame))
    {
        flags |= aiProcess_CalcTangentSpace;
    }

    return flags;
}

void AssimpImporter::PreImport_(
    Assimp::Importer&                  _importer,
    const Flags<eAssimpImporterFlags>& _flags) const
{
    if (_flags.IsEnabled(eAssimpImporterFlags::LoadAnimations))
    {
        _importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS);
    }
    else
    {
        _importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_LIGHTS | aiComponent_CAMERAS | aiComponent_BONEWEIGHTS | aiComponent_ANIMATIONS);
    }

    // remove line & point primitives
    _importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
    _importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
}

Vector<Ref<Mesh>> AssimpImporter::ConvertToMeshes_(
    const VertexLayout& _attribLayoutOrEmpty) const
{
    JAM_ASSERT(!m_importedMeshes.empty(), "AssimpImporter::ExtractToModel() - No meshes to create model");
    JAM_ASSERT(!_attribLayoutOrEmpty.Has(eVertexAttribute::Position), "Position attribute should not be included in the vertex layout, as it is provided separately.");

    // vertex layout에 존재하는 속성 확인
    const bool bHasNormals     = _attribLayoutOrEmpty.Has(eVertexAttribute::Normal);
    const bool bHasTangents    = _attribLayoutOrEmpty.Has(eVertexAttribute::Tangent);
    const bool bHasBitangents  = _attribLayoutOrEmpty.Has(eVertexAttribute::Bitangent);
    const bool bHasColor0      = _attribLayoutOrEmpty.Has(eVertexAttribute::Color0);
    const bool bHasTexCoord0   = _attribLayoutOrEmpty.Has(eVertexAttribute::TexCoord0);
    const bool bHasColor1      = _attribLayoutOrEmpty.Has(eVertexAttribute::Color1);
    const bool bHasTexCoord1   = _attribLayoutOrEmpty.Has(eVertexAttribute::TexCoord1);
    const bool bHasColor2      = _attribLayoutOrEmpty.Has(eVertexAttribute::Color2);
    const bool bHasTexCoord2   = _attribLayoutOrEmpty.Has(eVertexAttribute::TexCoord2);
    const bool bHasColor3      = _attribLayoutOrEmpty.Has(eVertexAttribute::Color3);
    const bool bHasTexCoord3   = _attribLayoutOrEmpty.Has(eVertexAttribute::TexCoord3);
    const bool bHasBlendIndex  = _attribLayoutOrEmpty.Has(eVertexAttribute::BlendIndex);
    const bool bHasBlendWeight = _attribLayoutOrEmpty.Has(eVertexAttribute::BlendWeight);

    // 첫 번째 메쉬를 기준으로 어떤 속성들이 임포트 되었는지 확인
    const ImportedMesh& firstMesh            = m_importedMeshes[0];
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

    if (_attribLayoutOrEmpty.IsEmpty())   // pos only mesh
    {
        Vector<Ref<Mesh>> meshes = {};
        meshes.reserve(m_importedMeshes.size());
        for (const ImportedMesh& importedMesh: m_importedMeshes)
        {
            Ref<Mesh> pMesh = Mesh::Load(importedMesh.positions, importedMesh.indexData);
            pMesh->SetName(importedMesh.name);
            meshes.emplace_back(pMesh);
        }
        return meshes;
    }
    else
    {
        Vector<Ref<Mesh>> meshes = {};
        meshes.reserve(m_importedMeshes.size());

        for (const ImportedMesh& importedMesh: m_importedMeshes)
        {
            const Int32 stride    = _attribLayoutOrEmpty.GetStride();
            const Int32 numVerts  = static_cast<Int32>(importedMesh.positions.size());
            const Int32 byteWidth = stride * numVerts;

            Vector<Byte> attribData = {};
            attribData.resize(byteWidth, 0);   // resize

            // vertex layout에 따라 속성 데이터 작성
            VertexWriter writer { attribData, _attribLayoutOrEmpty };
            for (Int32 i = 0; i < numVerts; ++i)
            {
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

            for (Int32 i = 0; i < numVerts; ++i)
            {
                if (bHasBlendIndex && !importedMesh.blendIndices.empty())
                    writer.WriteAt(i, eVertexAttribute::BlendIndex, importedMesh.blendIndices[i]);
                if (bHasBlendWeight && !importedMesh.blendWeights.empty())
                    writer.WriteAt(i, eVertexAttribute::BlendWeight, importedMesh.blendWeights[i]);
            }

            // create mesh
            Ref<Mesh> pMesh = Mesh::Load(importedMesh.positions, attribData, _attribLayoutOrEmpty, importedMesh.indexData);
            pMesh->SetName(importedMesh.name);
            meshes.emplace_back(pMesh);
        }

        return meshes;
    }
}

Expect<Vector<Ref<Texture>>> AssimpImporter::ConvertToTextures_() const
{
    JAM_ASSERT(!m_importedTextures.empty(), "AssimpImporter::ConvertToTextures_() - No textures to create model");

    Vector<Ref<Texture>> textures = {};
    textures.reserve(m_importedTextures.size());

    for (const ImportedTexture& importedTexture: m_importedTextures)
    {
        TextureHandle texh = kInvalidHandle;
        if (importedTexture.bIsCompressed)
        {
            // 타입에 맞게 import 필요
            Expect<Image> importer = Image::ImportFromMemory(importedTexture.pixels, importedTexture.format);
            if (!importer)
            {
                return importer.GetError();
            }

            // texture 생성
            Graphics& gfx = GetGraphics();
            texh          = gfx.CreateTexture(*importer);
        }
        else
        {
            // 비압축된 경우, 이미지 포맷은 BGRA8_UNorm 고정
            Graphics&        gfx    = GetGraphics();
            const MemoryView mems[] = { importedTexture.pixels };
            texh                    = gfx.CreateTexture2D(importedTexture.width, importedTexture.height, eTextureFormat::BGRA8_UNorm, false, 1, eTextureFlags::None, mems);
        }

        // texture 생성
        Ref<Texture> pTexture = Texture::Load(texh);
        pTexture->SetName(importedTexture.name);
        textures.emplace_back(pTexture);
    }

    return textures;
}

Vector<Ref<Material>> AssimpImporter::ConvertToMaterials_(
    const Vector<Ref<Texture>>& _texturesOrEmpty) const
{
    JAM_ASSERT(!m_importedMaterials.empty(), "AssimpImporter::ConvertToMaterials_() - No materials to create model");

    Vector<Ref<Material>> materials = {};
    materials.reserve(m_importedMaterials.size());

    for (const ImportedMaterial& importedMaterial: m_importedMaterials)
    {
        Ref<Material>& pMaterial = materials.emplace_back();
        pMaterial                = Material::Load();
        pMaterial->SetName(importedMaterial.name);

        pMaterial->SetParams(importedMaterial.params);
        pMaterial->SetSettings(importedMaterial.settings);

        // texture mapping
        if (!_texturesOrEmpty.empty())
        {
            for (eMaterialTexture type: RangesOf<eMaterialTexture>())
            {
                const Int32 index = importedMaterial.textures[type];
                if (index >= 0 && index < static_cast<Int32>(_texturesOrEmpty.size()))
                    pMaterial->SetTexture(type, _texturesOrEmpty[index]);
            }
        }
    }

    return materials;
}

void AssimpImporter::ImportMeshes_(
    const aiScene* _pScene)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportMeshesFrom_() - _pScene is nullptr");
    JAM_ASSERT(m_importedMeshes.empty(), "AssimpImporter::ImportMeshesFrom_() - m_meshes is not empty");

    if (_pScene->mNumMeshes == 0)
    {
        return;
    }

    const Int32 numMeshes = static_cast<Int32>(_pScene->mNumMeshes);
    m_importedMeshes.reserve(numMeshes);

    // import mesh
    for (Int32 i = 0; i < numMeshes; ++i)
    {
        ImportedMesh& importedMesh = m_importedMeshes.emplace_back();
        const aiMesh* pMesh        = _pScene->mMeshes[i];
        const Int32   numVerts     = static_cast<Int32>(pMesh->mNumVertices);

        // 이름
        {
            const aiString& aiName = pMesh->mName;
            importedMesh.name      = String::Formatted("{}_{}", aiName.Empty() ? "mesh" : aiName.C_Str(), i);
        }

        // 인덱스
        {
            // 인텍스 포맷
            importedMesh.bUseU32 = Greater(numVerts, Limits<UInt16>::kHighest);

            // 인덱스 버퍼 할당
            const Int32 numFaces  = static_cast<Int32>(pMesh->mNumFaces);
            const Int32 numIdxes  = numFaces * 3;
            const Int32 byteWidth = numIdxes * (importedMesh.bUseU32 ? sizeof(UInt32) : sizeof(UInt16));
            importedMesh.indexData.resize(byteWidth);

            // 인덱스 작성
            IndexWriter writer { importedMesh.indexData, importedMesh.bUseU32 };
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
                const Vec3 pos = AsVec3(pMesh->mVertices[v]);
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
                    const Vec2 uv = AsVec2(pMesh->mTextureCoords[j][k]);
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
                    const Vec4 col = AsVec4(pMesh->mColors[j][k]);
                    color.emplace_back(col);
                }
            }
        }

        // normals
        if (pMesh->HasNormals())
        {
            importedMesh.normals.reserve(numVerts);
            for (Int32 v = 0; v < numVerts; ++v)
            {
                const Vec3 normal = AsVec3(pMesh->mNormals[v]);
                importedMesh.normals.emplace_back(normal);
            }
        }

        // tangents & bitangents
        if (pMesh->HasTangentsAndBitangents())
        {
            importedMesh.tangents.reserve(numVerts);
            importedMesh.bitangents.reserve(numVerts);
            for (Int32 v = 0; v < numVerts; ++v)
            {
                const Vec3 tangent   = AsVec3(pMesh->mTangents[v]);
                const Vec3 bitangent = AsVec3(pMesh->mBitangents[v]);
                importedMesh.tangents.emplace_back(tangent);
                importedMesh.bitangents.emplace_back(bitangent);
            }
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

    const Int32 numTextures = static_cast<Int32>(_pScene->mNumTextures);
    m_importedTextures.reserve(numTextures);

    // 텍스처 순회
    for (Int32 i = 0; i < numTextures; ++i)
    {
        ImportedTexture& importedTexture = m_importedTextures.emplace_back();
        const aiTexture* pTexture        = _pScene->mTextures[i];
        const bool       bIsCompressed   = (pTexture->mHeight == 0);   // 압축된 텍스처는 mHeight가 0

        // 기본 정보
        importedTexture.width         = static_cast<Int32>(pTexture->mWidth);
        importedTexture.height        = static_cast<Int32>(pTexture->mHeight);
        importedTexture.bIsCompressed = bIsCompressed;

        // 이름 생성
        {
            const aiString& filename = pTexture->mFilename;
            importedTexture.name     = String::Formatted("{}_{}", filename.Empty() ? "texture" : filename.C_Str(), i);
        }

        // 픽셀 데이터
        if (bIsCompressed)
        {
            importedTexture.pixels = { pTexture->pcData, static_cast<Int32>(pTexture->mWidth) };

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
            const Int32 size       = static_cast<Int32>(importedTexture.width * importedTexture.height * sizeof(aiTexel));
            importedTexture.pixels = { pTexture->pcData, size };
        }

        // aiScene 해제 대비 복사
        const UInt8* pSrc = static_cast<const UInt8*>(importedTexture.pixels.GetPtr());
        importedTexture.storage.assign(pSrc, pSrc + importedTexture.pixels.GetByteWidth());
        importedTexture.pixels = { importedTexture.storage.data(), static_cast<SizeT>(importedTexture.storage.size()) };
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

    const Int32 numMats = static_cast<Int32>(_pScene->mNumMaterials);
    m_importedMaterials.reserve(numMats);

    // 머테리얼 순회
    for (Int32 i = 0; i < numMats; ++i)
    {
        ImportedMaterial& importedMaterial = m_importedMaterials.emplace_back();
        const aiMaterial* pMaterial        = _pScene->mMaterials[i];

        for (Int32& index: importedMaterial.textures)
            index = -1;

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

        // additional channel mapping
        if (pMaterial->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &strParam) == AI_SUCCESS)
        {
            importedMaterial.settings.roughnessMapChannel = eTextureChannel::Green;
            importedMaterial.settings.metallicMapChannel  = eTextureChannel::Blue;
        }

        // texture mapping
        if (Expect<Int32> index = ParseAsTextureIndex_(_pScene, pMaterial, aiTextureType_BASE_COLOR))
        {
            importedMaterial.textures[eMaterialTexture::Albedo] = index.GetValue();
        }

        if (importedMaterial.textures[eMaterialTexture::Albedo] < 0)
        {
            if (Expect<Int32> index = ParseAsTextureIndex_(_pScene, pMaterial, aiTextureType_DIFFUSE))
            {
                importedMaterial.textures[eMaterialTexture::Albedo] = index.GetValue();
            }
        }

        if (Expect<Int32> index = ParseAsTextureIndex_(_pScene, pMaterial, aiTextureType_NORMALS))
        {
            importedMaterial.textures[eMaterialTexture::Normal] = index.GetValue();
        }

        if (Expect<Int32> index = ParseAsTextureIndex_(_pScene, pMaterial, aiTextureType_METALNESS))
        {
            importedMaterial.textures[eMaterialTexture::Metallic] = index.GetValue();
        }

        if (Expect<Int32> index = ParseAsTextureIndex_(_pScene, pMaterial, aiTextureType_DIFFUSE_ROUGHNESS))
        {
            importedMaterial.textures[eMaterialTexture::Roughness] = index.GetValue();
        }

        if (Expect<Int32> index = ParseAsTextureIndex_(_pScene, pMaterial, aiTextureType_EMISSIVE))
        {
            importedMaterial.textures[eMaterialTexture::Emissive] = index.GetValue();
        }

        if (Expect<Int32> index = ParseAsTextureIndex_(_pScene, pMaterial, aiTextureType_LIGHTMAP))
        {
            importedMaterial.textures[eMaterialTexture::Light] = index.GetValue();
        }

        if (Expect<Int32> index = ParseAsTextureIndex_(_pScene, pMaterial, aiTextureType_AMBIENT_OCCLUSION))
        {
            importedMaterial.textures[eMaterialTexture::Ao] = index.GetValue();
        }

        if (Expect<Int32> index = ParseAsTextureIndex_(_pScene, pMaterial, aiTextureType_DISPLACEMENT))
        {
            importedMaterial.textures[eMaterialTexture::VertexDisplacement] = index.GetValue();
        }
    }
}

Error AssimpImporter::ImportAnimations_(
    const aiScene* _pScene)
{
    Skeleton& skeleton = m_animationData.skeleton;
    skeleton           = {};
    m_animationData.clips.clear();

    // 노드 평탄화 (부모 우선)
    HashMap<String, Int32>                  nodeIndexByName;
    Vector<Int32>                           meshNodeIndices(_pScene->mNumMeshes, -1);
    Vector<std::pair<const aiNode*, Int32>> stack;
    stack.emplace_back(_pScene->mRootNode, -1);

    while (!stack.empty())
    {
        const auto [pNode, parent] = stack.back();
        stack.pop_back();

        const Int32   index = static_cast<Int32>(skeleton.nodes.size());
        SkeletonNode& node  = skeleton.nodes.emplace_back();
        node.name           = String { pNode->mName.C_Str(), pNode->mName.length };
        node.parentIndex    = parent;
        node.bindLocal      = AsMtx4_(pNode->mTransformation);

        aiVector3D   scale, position;
        aiQuaternion rotation;
        pNode->mTransformation.Decompose(scale, rotation, position);
        node.bindScale       = AsVec3(scale);
        node.bindRotation    = Quat { rotation.x, rotation.y, rotation.z, rotation.w };
        node.bindTranslation = AsVec3(position);

        nodeIndexByName.emplace(node.name, index);
        for (UInt32 m = 0; m < pNode->mNumMeshes; ++m)
            if (meshNodeIndices[pNode->mMeshes[m]] < 0)
                meshNodeIndices[pNode->mMeshes[m]] = index;

        for (Int32 c = static_cast<Int32>(pNode->mNumChildren) - 1; c >= 0; --c)
            stack.emplace_back(pNode->mChildren[c], index);
    }

    // skin
    skeleton.meshSkins.resize(_pScene->mNumMeshes);
    for (UInt32 mi = 0; mi < _pScene->mNumMeshes; ++mi)
    {
        const aiMesh* pMesh = _pScene->mMeshes[mi];
        MeshSkin&     skin  = skeleton.meshSkins[mi];
        skin.meshNodeIndex  = meshNodeIndices[mi];
        if (pMesh->mNumBones == 0 || mi >= m_importedMeshes.size())
            continue;
        if (pMesh->mNumBones > static_cast<UInt32>(kMaxSkinJoints))
            return Error::ImportFailed;

        ImportedMesh& importedMesh = m_importedMeshes[mi];
        importedMesh.blendIndices.assign(pMesh->mNumVertices, Array<UInt8, 4> {});
        importedMesh.blendWeights.assign(pMesh->mNumVertices, Vec4 { 0.f, 0.f, 0.f, 0.f });
        Vector<Int32> influenceCount(pMesh->mNumVertices, 0);

        for (UInt32 bi = 0; bi < pMesh->mNumBones; ++bi)
        {
            const aiBone* pBone = pMesh->mBones[bi];
            const auto    it    = nodeIndexByName.find(String { pBone->mName.C_Str(), pBone->mName.length });
            if (it == nodeIndexByName.end())
                return Error::ImportFailed;

            SkinJoint& joint  = skin.joints.emplace_back();
            joint.nodeIndex   = it->second;
            joint.inverseBind = AsMtx4_(pBone->mOffsetMatrix);

            for (UInt32 wi = 0; wi < pBone->mNumWeights; ++wi)
            {
                const aiVertexWeight& vw = pBone->mWeights[wi];
                Int32&                n  = influenceCount[vw.mVertexId];
                if (n >= 4)
                    continue;
                importedMesh.blendIndices[vw.mVertexId][n] = static_cast<UInt8>(bi);
                importedMesh.blendWeights[vw.mVertexId][n] = vw.mWeight;
                ++n;
            }
        }

        // 가중치 정규화. 영향 없는 정점은 첫 조인트에 고정
        for (UInt32 v = 0; v < pMesh->mNumVertices; ++v)
        {
            Vec4&       w   = importedMesh.blendWeights[v];
            const float sum = w.x + w.y + w.z + w.w;
            w               = sum > 1e-6f ? w * (1.f / sum) : Vec4 { 1.f, 0.f, 0.f, 0.f };
        }
    }

    // clip
    for (UInt32 ai = 0; ai < _pScene->mNumAnimations; ++ai)
    {
        const aiAnimation* pAnim = _pScene->mAnimations[ai];
        const double       tps   = pAnim->mTicksPerSecond > 0.0 ? pAnim->mTicksPerSecond : 25.0;
        AnimationClip&     clip  = m_animationData.clips.emplace_back();
        clip.name                = String { pAnim->mName.C_Str(), pAnim->mName.length };
        clip.duration            = pAnim->mDuration / tps;

        for (UInt32 ci = 0; ci < pAnim->mNumChannels; ++ci)
        {
            const aiNodeAnim* pChannel = pAnim->mChannels[ci];
            const auto        it       = nodeIndexByName.find(String { pChannel->mNodeName.C_Str(), pChannel->mNodeName.length });
            if (it == nodeIndexByName.end())
                continue;

            AnimationChannel& channel = clip.channels.emplace_back();
            channel.nodeIndex         = it->second;
            for (UInt32 k = 0; k < pChannel->mNumPositionKeys; ++k)
                channel.translations.push_back({ pChannel->mPositionKeys[k].mTime / tps, AsVec3(pChannel->mPositionKeys[k].mValue) });
            for (UInt32 k = 0; k < pChannel->mNumRotationKeys; ++k)
            {
                const aiQuaternion& q = pChannel->mRotationKeys[k].mValue;
                channel.rotations.push_back({
                    pChannel->mRotationKeys[k].mTime / tps, Quat { q.x, q.y, q.z, q.w }
                });
            }
            for (UInt32 k = 0; k < pChannel->mNumScalingKeys; ++k)
                channel.scales.push_back({ pChannel->mScalingKeys[k].mTime / tps, AsVec3(pChannel->mScalingKeys[k].mValue) });
        }
    }

    return Error::None;
}

void AssimpImporter::ImportModelNode_(
    const aiScene* _pScene)
{
    JAM_ASSERT(_pScene != nullptr, "AssimpImporter::ImportModelNode_() - _pScene is nullptr");
    m_importedRoot = ImportModelNode_Recursive_(_pScene, _pScene->mRootNode);
}

ModelNode AssimpImporter::ImportModelNode_Recursive_(
    const aiScene* _pScene,
    const aiNode*  _pNode)
{
    ModelNode node = {};

    // 이름
    node.name = String { _pNode->mName.C_Str(), _pNode->mName.length };

    // 행렬
    {
        const aiMatrix4x4& mtx = _pNode->mTransformation;
        node.localMtx.r0       = { mtx.a1, mtx.b1, mtx.c1, mtx.d1 };
        node.localMtx.r1       = { mtx.a2, mtx.b2, mtx.c2, mtx.d2 };
        node.localMtx.r2       = { mtx.a3, mtx.b3, mtx.c3, mtx.d3 };
        node.localMtx.r3       = { mtx.a4, mtx.b4, mtx.c4, mtx.d4 };
    }

    // 메쉬와 메쉬의 머테리얼 인덱스
    const Int32 numMeshes = static_cast<Int32>(_pNode->mNumMeshes);
    if (numMeshes > 0)
    {
        node.meshIdxes.reserve(numMeshes);
        node.matIdxes.reserve(numMeshes);

        // 메쉬는 반드시 하나의 머테리얼과 연관되어있음
        for (Int32 i = 0; i < numMeshes; ++i)
        {
            const Int32   meshIdx = static_cast<Int32>(_pNode->mMeshes[i]);
            const aiMesh* pMesh   = _pScene->mMeshes[meshIdx];
            const Int32   matIdx  = static_cast<Int32>(pMesh->mMaterialIndex);

            node.meshIdxes.emplace_back(meshIdx);
            node.matIdxes.emplace_back(matIdx);
        }
    }

    // 하위 자식 노드들
    const Int32 numChildren = static_cast<Int32>(_pNode->mNumChildren);
    node.children.reserve(numChildren);
    for (Int32 i = 0; i < numChildren; ++i)
    {
        node.children.emplace_back(ImportModelNode_Recursive_(_pScene, _pNode->mChildren[i]));
    }

    return node;
}

}   // namespace Jam
