#pragma once

#include "EnumArray.h"
#include "Material.h"
#include "Model.h"
#include "SkeletonAnimation.h"

namespace Assimp
{
class Importer;
}

struct aiNode;
struct aiTexture;
struct aiMaterial;
struct aiScene;
struct aiMesh;

namespace Jam
{

enum class eImageFormat;
struct ModelNode;
class Texture;
class Material;
class Model;
class Mesh;

struct ImportedMesh
{
    String name = {};

    Vector<Vec3>            positions  = {};
    Vector<Vec3>            normals    = {};
    Vector<Vec3>            tangents   = {};
    Vector<Vec3>            bitangents = {};
    Array<Vector<Vec4>, 4>  colors     = {};
    Array<Vector<Vec2>, 4>  texCoords  = {};
    Vector<Byte>            indexData  = {};
    bool                    bUseU32    = false;
    Vector<Array<UInt8, 4>> blendIndices;
    Vector<Vec4>            blendWeights;
};

struct ImportedMaterial
{
    String                             name     = {};
    MaterialParams                     params   = {};
    EnumArray<eMaterialTexture, Int32> textures = {};
    MaterialTextureSettings            settings = {};
};

struct ImportedTexture
{
    String        name          = {};
    Vector<UInt8> storage       = {};   // pixels 원본 (복사본)
    MemoryView    pixels        = {};
    Int32         width         = 0;
    Int32         height        = 0;
    eImageFormat  format        = {};
    bool          bIsCompressed = false;
};

enum class eAssimpImporterFlags : UInt32
{
    None                 = 0,
    GenerateNormals      = 1 << 0,
    GenerateTangentFrame = 1 << 1,
    DropExistingNormals  = 1 << 2,
    LimitBoneWeights     = 1 << 3,
    LoadStaticMesh       = 1 << 4,

    LoadMeshes    = 1 << 5,
    LoadMaterials = 1 << 6,
    LoadTextures  = 1 << 7,

    GenerateSmoothNormals = 1 << 8,
    LoadAnimations        = 1 << 9,
};

enum class eAssimpConvertToModelFlags : UInt32
{
    None = 0,

    Mesh     = 1 << 0,
    Material = 1 << 1,
    Texture  = 1 << 2,
    All      = Mesh | Material | Texture,

    NoInvalidate = 1 << 3,   // 변환 후에도 모델 데이터를 무효화하지 않음. 기본 동작은 무효화이며, 이 경우가 성능적으로 더 빠름.
};

class AssimpImporter
{
public:
    [[nodiscard]] static Expect<AssimpImporter> ImportFromMemory(MemoryView _data, Flags<eAssimpImporterFlags> _flags);
    [[nodiscard]] static Expect<AssimpImporter> ImportFromFile(StringView _path, Flags<eAssimpImporterFlags> _flags);

    [[nodiscard]] const ModelNode&                GetRoot() const;
    [[nodiscard]] const Vector<ImportedMesh>&     GetMeshes() const;
    [[nodiscard]] const Vector<ImportedMaterial>& GetMaterials() const;
    [[nodiscard]] const Vector<ImportedTexture>&  GetTextures() const;
    [[nodiscard]] const SkeletalAnimationData&    GetAnimationData() const;

    [[nodiscard]] Expect<Ref<Model>> ConvertToModel(const VertexLayout& _attribLayoutOrEmpty, Flags<eAssimpConvertToModelFlags> _flags = eAssimpConvertToModelFlags::All);

private:
    AssimpImporter() = default;
    Error  ImportFromMemory_(MemoryView _data, const Flags<eAssimpImporterFlags>& _flags);
    Error  ImportFromFile_(StringView _path, const Flags<eAssimpImporterFlags>& _flags);
    Error  Import_(const aiScene* _pScene, Flags<eAssimpImporterFlags> _flags);
    UInt32 MakeImportFlags_(const Flags<eAssimpImporterFlags>& _flags) const;
    void   PreImport_(Assimp::Importer& _importer, const Flags<eAssimpImporterFlags>& _flags) const;

    // import steps
    void                    ImportMeshes_(const aiScene* _pScene);
    Error                   ImportTextures_(const aiScene* _pScene);
    void                    ImportMaterials_(const aiScene* _pScene);
    void                    ImportModelNode_(const aiScene* _pScene);
    Error                   ImportAnimations_(const aiScene* _pScene);
    [[nodiscard]] ModelNode ImportModelNode_Recursive_(const aiScene* _pScene, const aiNode* _pNode);

    // convert
    [[nodiscard]] Vector<Ref<Mesh>>            ConvertToMeshes_(const VertexLayout& _attribLayoutOrEmpty) const;
    [[nodiscard]] Expect<Vector<Ref<Texture>>> ConvertToTextures_() const;
    [[nodiscard]] Vector<Ref<Material>>        ConvertToMaterials_(const Vector<Ref<Texture>>& _texturesOrEmpty) const;

    ModelNode                m_importedRoot      = {};
    Vector<ImportedMesh>     m_importedMeshes    = {};
    Vector<ImportedMaterial> m_importedMaterials = {};
    Vector<ImportedTexture>  m_importedTextures  = {};
    SkeletalAnimationData    m_animationData;
};

}   // namespace Jam
