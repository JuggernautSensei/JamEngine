#include "pch.h"

#include "Mesh.h"

#include "BaseUtilities.h"
#include "Gfx.h"
#include "Memory.h"
#include "Profile.h"
#include "resources/flatc/jmsh_generated.h"
#include <flatbuffers/flatbuffers.h>
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

constexpr eVertexAttribute k_nativeVertexAttribteTable[jmsh::eVertexAttribute_MAX + 1] = {
    eVertexAttribute::Unknown,       // eVertexAttribute_UnknownAttribute
    eVertexAttribute::Position,      // eVertexAttribute_Position
    eVertexAttribute::Normal,        // eVertexAttribute_Normal
    eVertexAttribute::Tangent,       // eVertexAttribute_Tangent
    eVertexAttribute::Bitangent,     // eVertexAttribute_Bitangent
    eVertexAttribute::Color0,        // eVertexAttribute_Color0
    eVertexAttribute::Color1,        // eVertexAttribute_Color1
    eVertexAttribute::Color2,        // eVertexAttribute_Color2
    eVertexAttribute::Color3,        // eVertexAttribute_Color3
    eVertexAttribute::BlendIndex,    // eVertexAttribute_BlendIndices
    eVertexAttribute::BlendWeight,   // eVertexAttribute_BlendWeights
    eVertexAttribute::TexCoord0,     // eVertexAttribute_TexCoord0
    eVertexAttribute::TexCoord1,     // eVertexAttribute_TexCoord1
    eVertexAttribute::TexCoord2,     // eVertexAttribute_TexCoord2
    eVertexAttribute::TexCoord3,     // eVertexAttribute_TexCoord3
};

constexpr jmsh::eVertexAttribute k_fbsVertexAttributeTable[EnumCount<eVertexAttribute>()] = {
    jmsh::eVertexAttribute::eVertexAttribute_Position,       // eVertexAttribute::Position
    jmsh::eVertexAttribute::eVertexAttribute_Normal,         // eVertexAttribute::Normal
    jmsh::eVertexAttribute::eVertexAttribute_Tangent,        // eVertexAttribute::Tangent
    jmsh::eVertexAttribute::eVertexAttribute_Bitangent,      // eVertexAttribute::Bitangent
    jmsh::eVertexAttribute::eVertexAttribute_Color0,         // eVertexAttribute::Color0
    jmsh::eVertexAttribute::eVertexAttribute_Color1,         // eVertexAttribute::Color0
    jmsh::eVertexAttribute::eVertexAttribute_Color2,         // eVertexAttribute::Color0
    jmsh::eVertexAttribute::eVertexAttribute_Color3,         // eVertexAttribute::Color0
    jmsh::eVertexAttribute::eVertexAttribute_BlendIndices,   // eVertexAttribute::BlendIndices
    jmsh::eVertexAttribute::eVertexAttribute_BlendWeights,   // eVertexAttribute::BlendWeights
    jmsh::eVertexAttribute::eVertexAttribute_TexCoord0,      // eVertexAttribute::TexCoord0
    jmsh::eVertexAttribute::eVertexAttribute_TexCoord1,      // eVertexAttribute::TexCoord0
    jmsh::eVertexAttribute::eVertexAttribute_TexCoord2,      // eVertexAttribute::TexCoord0
    jmsh::eVertexAttribute::eVertexAttribute_TexCoord3,      // eVertexAttribute::TexCoord0
};

constexpr eAttributeFormat k_nativeAttributeFormatTable[jmsh::eAttributeFormat_MAX + 1] = {
    eAttributeFormat::Unknown,    // eAttributeFormat_UnknownFormat
    eAttributeFormat::Float,      // eAttributeFormat_Float
    eAttributeFormat::Float2,     // eAttributeFormat_Float2
    eAttributeFormat::Float3,     // eAttributeFormat_Float3
    eAttributeFormat::Float4,     // eAttributeFormat_Float4
    eAttributeFormat::UNorm8x4,   // eAttributeFormat_UNorm8x4
    eAttributeFormat::UInt8x4     // eAttributeFormat_UInt8x4
};

constexpr jmsh::eAttributeFormat k_fbsAttributeFormatTable[EnumCount<eAttributeFormat>()] = {
    jmsh::eAttributeFormat::eAttributeFormat_Float,      // eAttributeFormat::Float
    jmsh::eAttributeFormat::eAttributeFormat_Float2,     // eAttributeFormat::Float2
    jmsh::eAttributeFormat::eAttributeFormat_Float3,     // eAttributeFormat::Float3
    jmsh::eAttributeFormat::eAttributeFormat_Float4,     // eAttributeFormat::Float4
    jmsh::eAttributeFormat::eAttributeFormat_UNorm8x4,   // eAttributeFormat::UNorm8x4
    jmsh::eAttributeFormat::eAttributeFormat_UInt8x4     // eAttributeFormat::UInt8x4
};

JAM_NODISCARD static Vec3 ConvertToNativeVec3(const jmsh::Vec3& _vec)
{
    return { _vec.x(), _vec.y(), _vec.z() };
}

JAM_NODISCARD static jmsh::Vec3 ConvertToFbsVec3(const Vec3& _vec)
{
    return { _vec.x, _vec.y, _vec.z };
}

////////////////////////////////////////////////////////////

namespace jam
{

eErrorCode Mesh::Load(const fs::path& _path)
{
    JAM_PROFILE_SCOPE_MS(R"(jam::Mesh::Load "{}")", _path.native());

    CleanUp_();

    // 파일 io
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs.is_open())
    {
        SetErrorString(R"(Failed to open mesh file: "{}")", _path.native());
        return eErrorCode::FileOpenFailed;
    }

    // file size
    std::istream& eof  = ifs.seekg(0, std::ios::end);
    size_t        size = eof.tellg();
    ifs.seekg(0, std::ios::beg);

    // file read
    std::vector<char> buffer;
    buffer.resize(size);
    ifs.read(buffer.data(), size);
    if (ifs.fail() || ifs.bad())
    {
        SetErrorString(R"(Failed to read mesh file: "{}")", _path.native());
        return eErrorCode::FileReadFailed;
    }

    // verify
    flatbuffers::Verifier verifier { reinterpret_cast<const uint8_t*>(buffer.data()), size };
    if (!jmsh::VerifyMeshBuffer(verifier))
    {
        SetErrorString(R"(Mesh file is corrupted or invalid: "{}")", _path.native());
        return eErrorCode::FileVertifyFailed;
    }

    // mesh parse
    const jmsh::Mesh* pMesh = jmsh::GetMesh(buffer.data());
    if (!pMesh)
    {
        SetErrorString(R"(Failed to parse mesh file: "{}")", _path.native());
        return eErrorCode::FileParseFailed;
    }

    // version parse
    Version version { pMesh->version() };
    if (version.major != k_version.major)
    {
        SetErrorString(R"(Mesh file major version is different: "{}" (file version: {}, engine version: {}))", _path.native(), version.ToString(), k_version.ToString());
        return eErrorCode::VersionMismatch;
    }

    if (version.minor != k_version.minor)
    {
        logger::Warn("Mesh file minor version is different: {} (file version: {}, engine version: {})", _path.native(), version.ToString(), k_version.ToString());
    }

    // vertex layout
    const jmsh::VertexLayout* pLayout = pMesh->vertex_layout();
    if (!pLayout)
    {
        SetErrorString(R"(Failed to parse vertex layout in mesh file: "{}")", _path.native());
        return eErrorCode::FileParseFailed;
    }

    // vertex layout element parse
    const auto* pLayoutElems = pLayout->elems();
    if (!pLayoutElems || pLayoutElems->size() == 0)
    {
        SetErrorString(R"(Vertex layout in mesh file is empty: "{}")", _path.native());
        return eErrorCode::FileParseFailed;
    }

    // native vertex layout 생성
    VertexLayout layout;
    for (Int32 i = 0; i < pLayout->elems()->size(); ++i)
    {
        const jmsh::VertexLayoutElement* pElem = pLayoutElems->Get(i);
        if (!pElem)
        {
            SetErrorString(R"(Failed to parse vertex layout element in mesh file: "{}")", _path.native());
            return eErrorCode::FileParseFailed;
        }
        layout.Add(k_nativeVertexAttribteTable[pElem->attrib()], k_nativeAttributeFormatTable[pElem->format()]);
    }

    // vertex, index 데이터
    auto* pVertices = pMesh->vertices();
    if (!pVertices || pVertices->size() == 0)
    {
        SetErrorString(R"(Vertex data in mesh file is empty: "{}")", _path.native());
        return eErrorCode::FileParseFailed;
    }

    auto* pIndices = pMesh->indices();
    if (!pIndices || pIndices->size() == 0)
    {
        SetErrorString(R"(Index data in mesh file is empty: "{}")", _path.native());
        return eErrorCode::FileParseFailed;
    }

    // vertex, index MemoryView 생성
    MemoryView vertices { pMesh->vertices()->data(), pMesh->vertices()->size() };
    MemoryView indices { pMesh->indices()->data(), pMesh->indices()->size() };

    // index format 결정
    Int32        vertexCount = pMesh->vertices()->size() / layout.GetStride();
    eIndexFormat indexFormat = std::cmp_less_equal(vertexCount, UInt16Limits::max() ? eIndexFormat::U16 : eIndexFormat::U32;

    // gpu 리소스 생성
    m_path               = _path;
    m_vertexBufferHandle = gfx::CreateVertexBuffer(vertices, layout);
    m_indexBufferHandle  = gfx::CreateIndexBuffer(indices, indexFormat);

    // 바운딩 박스 설정
    const auto* pAABB = pMesh->aabb();
    if (pAABB)
    {
        m_boundingBox.Center  = ConvertToNativeVec3(pAABB->center());
        m_boundingBox.Extents = ConvertToNativeVec3(pAABB->extents());
    }
    else
    {
        // aabb 정보 없으면 직접 계산
        CalcBoundingBox_(vertices, layout);
    }

    // 완료
    return eErrorCode::Ok;
}

void Mesh::Load(
    const MemoryView    _vertices,
    const VertexLayout& _layout,
    const MemoryView    _indices,
    const eIndexFormat  _format)
{
    JAM_PROFILE_SCOPE_MS(R"(jam::Mesh::Load from memory)");

    // 기존 리소스 정리
    CleanUp_();

    // gpu 리소스 생성
    m_vertexBufferHandle = gfx::CreateVertexBuffer(_vertices, _layout);
    m_indexBufferHandle  = gfx::CreateIndexBuffer(_indices, _format);

    // 바운딩 박스 계산
    CalcBoundingBox_(_vertices, _layout);
}

eErrorCode Mesh::Save(const fs::path& _path) const
{
    JAM_ASSERT(IsLoaded(), "Mesh is not loaded");
    JAM_PROFILE_SCOPE_MS(R"(jam::Mesh::Save "{}")", _path.native());

    flatbuffers::FlatBufferBuilder builder(1024);

    // vertex layout
    const VertexLayout&                       vertexLayout = gfx::GetVertexLayout(m_vertexBufferHandle);
    std::array<jmsh::VertexLayoutElement, 14> fbsLayoutElems;
    Int32                                     elemCount = 0;
    for (const VertexLayoutElement& elem: vertexLayout.GetElements())
    {
        fbsLayoutElems[elemCount++] = jmsh::VertexLayoutElement {
            k_fbsVertexAttributeTable[EnumToInt(elem.attrib)],
            k_fbsAttributeFormatTable[EnumToInt(elem.format)],
            NarrowCast<Int16>(elem.offset)
        };
    }
    jmsh::VertexLayout pFbsLayout { fbsLayoutElems };

    // vertex buffer
    std::vector<Byte> verticesData = gfx::DownloadBuffer(m_vertexBufferHandle);
    auto              fbsVertices  = builder.CreateVector(verticesData);

    // index buffer
    std::vector<Byte> indicesData = gfx::DownloadBuffer(m_indexBufferHandle);
    auto              fbsIndices  = builder.CreateVector(indicesData);

    // aabb
    jmsh::AABB fbsAABB(ConvertToFbsVec3(m_boundingBox.Center), ConvertToFbsVec3(m_boundingBox.Extents));

    // mesh
    Int32 version = k_version.Pack();   // 버전
    auto  fbsMesh = jmsh::CreateMesh(builder, version, fbsVertices, &pFbsLayout, fbsIndices, &fbsAABB);
    builder.Finish(fbsMesh);

    Byte*  buf  = builder.GetBufferPointer();
    size_t size = builder.GetSize();

    std::ofstream ofs(_path, std::ios::binary);
    if (!ofs.is_open())
    {
        SetErrorString(R"(Failed to open mesh file for writing: "{}")", _path.native());
        return eErrorCode::FileOpenFailed;
    }

    ofs.write(reinterpret_cast<const char*>(buf), size);
    if (ofs.fail() || ofs.bad())
    {
        SetErrorString(R"(Failed to write mesh file: "{}")", _path.native());
        return eErrorCode::FileWriteFailed;
    }
    return eErrorCode::Ok;
}

void Mesh::CalcBoundingBox_(const MemoryView _vertices, const VertexLayout& _layout)
{
    // 포지션 속성 정보
    JAM_ASSERT(_layout.Has(eVertexAttribute::Position), "Position attribute is required to calculate bounding box");
    const VertexLayoutElement& posElement = _layout.GetElement(eVertexAttribute::Position);

    Int32 byteWidth = Min(posElement.byteWidth, sizeof(Vec3));
    Int32 offset    = posElement.offset;
    Int32 stride    = _layout.GetStride();

    // 버텍스 정보
    Int32       vertexCount = NarrowCast<Int32>(_vertices.GetByteWidth() / stride);
    const Byte* pVertexData = _vertices.GetDataPtr();

    Vec3 minPos { FloatLimits::max(), FloatLimits::max(), FloatLimits::max() };
    Vec3 maxPos { -FloatLimits::lowest(), -FloatLimits::lowest(), -FloatLimits::lowest() };
    for (Int32 i = 0; i < vertexCount; ++i)
    {
        const Byte* pSrc = pVertexData + i * stride + offset;

        Vec3 pos = Vec3::Zero;
        std::memcpy(&pos, pSrc, byteWidth);

        minPos.x = Min(minPos.x, pos.x);
        minPos.y = Min(minPos.y, pos.y);
        minPos.z = Min(minPos.z, pos.z);
        maxPos.x = Max(maxPos.x, pos.x);
        maxPos.y = Max(maxPos.y, pos.y);
        maxPos.z = Max(maxPos.z, pos.z);
    }

    Vec3 center   = (minPos + maxPos) * 0.5f;
    Vec3 extents  = (maxPos - minPos) * 0.5f;
    m_boundingBox = AABB { center, extents };
}

Mesh::~Mesh()
{
    CleanUp_();
}

Mesh::Mesh(Mesh&& _other) noexcept
    : m_path(std::move(_other.m_path))
    , m_vertexBufferHandle(_other.m_vertexBufferHandle)
    , m_indexBufferHandle(_other.m_indexBufferHandle)
    , m_boundingBox(std::move(_other.m_boundingBox))
{
    _other.m_indexBufferHandle  = k_invalidHandle;
    _other.m_vertexBufferHandle = k_invalidHandle;
    _other.m_boundingBox        = {};
    _other.m_path.clear();
}

Mesh& Mesh::operator=(Mesh&& _other) noexcept
{
    if (this != &_other)
    {
        Mesh temp(std::move(_other));
        std::swap(*this, temp);
    }
    return *this;
}

void Mesh::Unload()
{
    CleanUp_();
}

bool Mesh::IsLoaded() const
{
    return m_vertexBufferHandle != k_invalidHandle
        && m_indexBufferHandle != k_invalidHandle;
}

const fs::path& Mesh::GetPath() const
{
    return m_path;
}

eAssetType Mesh::GetType() const
{
    return k_type;
}

UInt32 Mesh::GetHash() const
{
    return k_hash;
}

std::string_view Mesh::GetName() const
{
    return k_name;
}

void Mesh::Submit() const
{
    gfx::SetVertexBuffer(m_vertexBufferHandle);
    gfx::SetIndexBuffer(m_indexBufferHandle);
    gfx::Submit();
}

bool Mesh::Intersect(const Mat4& _meshTransform, const Frustum& _frustumInWorld) const
{
    AABB aabbInWorld;
    m_boundingBox.Transform(aabbInWorld, _meshTransform);
    return _frustumInWorld.Intersects(aabbInWorld);
}

bool Mesh::Intersect(const Mat4& _meshTransform, const AABB& _aabbInWorld) const
{
    AABB aabbInWorld;
    m_boundingBox.Transform(aabbInWorld, _meshTransform);
    return aabbInWorld.Intersects(_aabbInWorld);
}

bool Mesh::Intersect(const Mat4& _meshTransform, const Sphere& _sphereInWorld) const
{
    AABB aabbInWorld;
    m_boundingBox.Transform(aabbInWorld, _meshTransform);
    return aabbInWorld.Intersects(_sphereInWorld);
}

bool Mesh::Intersect(const Mat4& _meshTransform, const OBB& _obbInWorld) const
{
    AABB aabbInWorld;
    m_boundingBox.Transform(aabbInWorld, _meshTransform);
    return aabbInWorld.Intersects(_obbInWorld);
}

IntersectResult Mesh::Intersect(const Mat4& _meshTransform, const Ray& _rayInWorld) const
{
    AABB aabbInWorld;
    m_boundingBox.Transform(aabbInWorld, _meshTransform);

    float distance;
    bool  bResult = _rayInWorld.Intersects(aabbInWorld, distance);
    return { bResult, distance };
}

VertexBufferHandle Mesh::GetVertexBuffer() const
{
    return m_vertexBufferHandle;
}

IndexBufferHandle Mesh::GetIndexBuffer() const
{
    return m_indexBufferHandle;
}

const AABB& Mesh::GetBoundingBox() const
{
    return m_boundingBox;
}

void Mesh::CleanUp_()
{
    JAM_DESTROY_VERTEX_BUFFER(m_vertexBufferHandle);
    JAM_DESTROY_INDEX_BUFFER(m_indexBufferHandle);
    m_boundingBox = {};
    m_path.clear();
}

}   // namespace jam