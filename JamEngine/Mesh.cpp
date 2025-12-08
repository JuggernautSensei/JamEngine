#include "pch.h"

#include "Mesh.h"

#include "BaseUtilities.h"
#include "FileUtilities.h"
#include "Graphics.h"
#include "Hash.h"
#include "Profile.h"
#include "resources/flatc/jmsh_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace jam
{

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

constexpr jmsh::eVertexAttribute k_fbsVertexAttributeTable[] = {
    jmsh::eVertexAttribute::eVertexAttribute_Position,       // eVertexAttribute::Position
    jmsh::eVertexAttribute::eVertexAttribute_Normal,         // eVertexAttribute::Normal
    jmsh::eVertexAttribute::eVertexAttribute_Tangent,        // eVertexAttribute::Tangent
    jmsh::eVertexAttribute::eVertexAttribute_Bitangent,      // eVertexAttribute::Bitangent
    jmsh::eVertexAttribute::eVertexAttribute_Color0,         // eVertexAttribute::Color0
    jmsh::eVertexAttribute::eVertexAttribute_Color1,         // eVertexAttribute::Color1
    jmsh::eVertexAttribute::eVertexAttribute_Color2,         // eVertexAttribute::Color2
    jmsh::eVertexAttribute::eVertexAttribute_Color3,         // eVertexAttribute::Color3
    jmsh::eVertexAttribute::eVertexAttribute_BlendIndices,   // eVertexAttribute::BlendIndices
    jmsh::eVertexAttribute::eVertexAttribute_BlendWeights,   // eVertexAttribute::BlendWeights
    jmsh::eVertexAttribute::eVertexAttribute_TexCoord0,      // eVertexAttribute::TexCoord0
    jmsh::eVertexAttribute::eVertexAttribute_TexCoord1,      // eVertexAttribute::TexCoord1
    jmsh::eVertexAttribute::eVertexAttribute_TexCoord2,      // eVertexAttribute::TexCoord2
    jmsh::eVertexAttribute::eVertexAttribute_TexCoord3,      // eVertexAttribute::TexCoord3
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

constexpr jmsh::eAttributeFormat k_fbsAttributeFormatTable[] = {
    jmsh::eAttributeFormat::eAttributeFormat_Float,      // eAttributeFormat::Float
    jmsh::eAttributeFormat::eAttributeFormat_Float2,     // eAttributeFormat::Float2
    jmsh::eAttributeFormat::eAttributeFormat_Float3,     // eAttributeFormat::Float3
    jmsh::eAttributeFormat::eAttributeFormat_Float4,     // eAttributeFormat::Float4
    jmsh::eAttributeFormat::eAttributeFormat_UNorm8x4,   // eAttributeFormat::UNorm8x4
    jmsh::eAttributeFormat::eAttributeFormat_UInt8x4     // eAttributeFormat::UInt8x4
};

[[nodiscard]] static Vec3 ToVec3(const jmsh::Vec3 _vec)
{
    return Vec3 { _vec.x(), _vec.y(), _vec.z() };
}

[[nodiscard]] static jmsh::Vec3 ToJmshVec3(const Vec3 _vec)
{
    return jmsh::Vec3 { _vec.x, _vec.y, _vec.z };
}

}   // namespace jam

namespace jam
{

bool Mesh::Load(const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    CleanUp_();

    // 파일 io
    auto optBuffer = LoadBinaryFromFile(_path);
    if (!optBuffer)
    {
        return false;
    }
    const std::vector<Byte>& buffer = *optBuffer;

    // verify
    flatbuffers::Verifier verifier { buffer.data(), buffer.size() };
    if (!jmsh::VerifyMeshBuffer(verifier))
    {
        SetErrorString(R"(Mesh file is corrupted or invalid: "{}")", _path.native());
        return false;
    }

    // mesh parse
    const jmsh::Mesh* pFbsMesh = jmsh::GetMesh(buffer.data());
    if (!pFbsMesh)
    {
        SetErrorString(R"(Failed to parse mesh file: "{}")", _path.native());
        return false;
    }

    // vertex vertexLayout
    const jmsh::VertexLayout* pFbsVertexLayout = pFbsMesh->vertex_layout();
    if (!pFbsVertexLayout)
    {
        SetErrorString(R"(Failed to parse vertex vertexLayout in mesh file: "{}")", _path.native());
        return false;
    }

    // vertex vertexLayout element parse
    const auto* pElems   = pFbsVertexLayout->elems();
    const Int32 numElems = pFbsVertexLayout->count();
    if (!pElems || numElems == 0)
    {
        SetErrorString(R"(Vertex vertexLayout in mesh file is empty: "{}")", _path.native());
        return false;
    }

    // build vertex vertexLayout
    VertexLayout vertLayout;
    for (Int32 i = 0; i < numElems; ++i)
    {
        if (const jmsh::VertexLayoutElement* pElem = pElems->Get(i))
        {
            const eVertexAttribute attrib = k_nativeVertexAttribteTable[pElem->attrib()];
            const eAttributeFormat format = k_nativeAttributeFormatTable[pElem->format()];
            vertLayout.Add(attrib, format);
        }
        else
        {
            SetErrorString(R"(Failed to parse vertex vertexLayout element in mesh file: "{}")", _path.native());
            return false;
        }
    }

    // vertex / index data
    const auto* pFbsVerts = pFbsMesh->vertices();
    if (!pFbsVerts || pFbsVerts->size() == 0)
    {
        SetErrorString(R"(Vertex data in mesh file is empty: "{}")", _path.native());
        return false;
    }

    const auto* pFbsIndices = pFbsMesh->indices();
    if (!pFbsIndices || pFbsIndices->size() == 0)
    {
        SetErrorString(R"(Index data in mesh file is empty: "{}")", _path.native());
        return false;
    }

    // vertex, index MemoryView 생성
    ConstMemView verts { pFbsVerts->data(), pFbsVerts->size() };
    ConstMemView indices { pFbsIndices->data(), pFbsIndices->size() };

    // GPU resource creation
    {
        // index format
        const Int32        numVerts    = NarrowCast<Int32>(pFbsVerts->size() / vertLayout.GetStride());
        const eIndexFormat indexFormat = NeedU32Index(numVerts);

        m_path = _path;
        m_vbh  = Graphics::CreateVertexBuffer(verts, vertLayout);
        m_ibh  = Graphics::CreateIndexBuffer(indices, indexFormat);
    }

    // bounding box
    {
        const auto* pAabb = pFbsMesh->aabb();
        if (pAabb)
        {
            m_boundingBox.min = ToVec3(pAabb->min());
            m_boundingBox.max = ToVec3(pAabb->max());
        }
        else
        {
            // compute if absent`
            ComputeBoundingBox_(verts, vertLayout);
        }
    }

    // update hash based on file path
    {
        const std::string_view strPath = _path.native();

        MurmurHash64 hasher;
        hasher.Begin();
        hasher.Add(strPath.data(), strPath.size());
        m_sortKey = hasher.End() & 0x0000FFFF;   // use lower 16 bits
    }

    return true;
}

void Mesh::LoadFromMemory(
    const ConstMemView      _vertices,
    const VertexLayout& _layout,
    const ConstMemView      _indices,
    const eIndexFormat  _format,
    const fs::path&     _virtualPath)
{
    JAM_FUNCTION_PROFILE();
    CleanUp_();

    // create GPU resources
    {
        m_vbh = Graphics::CreateVertexBuffer(_vertices, _layout);
        m_ibh = Graphics::CreateIndexBuffer(_indices, _format);
    }

    // compute bounding box
    {
        ComputeBoundingBox_(_vertices, _layout);
    }

    // update sort key
    MurmurHash64 hasher;
    hasher.Begin();
    if (m_path.empty())
    {
        // hash based on data
        hasher.Add(_vertices.GetDataPtr(), _vertices.GetByteWidth());
        hasher.Add(&_layout, sizeof(VertexLayout));
        hasher.Add(_indices.GetDataPtr(), _indices.GetByteWidth());
        hasher.Add(&_format, sizeof(eIndexFormat));
    }
    else
    {
        // hash based on virtual path
        const std::string_view strPath = _virtualPath.native();
        hasher.Add(strPath.data(), strPath.size());
    }
    const UInt64 hash = hasher.End();
    m_sortKey         = hash & 0xFFFF;   // use lower 16 bits
}

bool Mesh::Save(const fs::path& _path) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());

    JAM_ASSERT(IsValid(), "Mesh is not loaded");
    flatbuffers::FlatBufferBuilder builder(1024);

    // vertex vertexLayout -> flatbuffers
    const VertexLayout&       vertexLayout = Graphics::GetVertexLayout(m_vbh);
    jmsh::VertexLayoutElement fbsElems[14] = {};
    Int32                     numElems     = 0;
    for (const VertexLayoutElement& elem: vertexLayout.GetElements())
    {
        const jmsh::eVertexAttribute fbsAttrib = k_fbsVertexAttributeTable[EnumToInt(elem.attrib)];
        const jmsh::eAttributeFormat fbsFormat = k_fbsAttributeFormatTable[EnumToInt(elem.format)];
        const Int16                  offset    = NarrowCast<Int16>(elem.offset);

        fbsElems[numElems] = jmsh::VertexLayoutElement { fbsAttrib, fbsFormat, offset };
        ++numElems;
    }
    jmsh::VertexLayout pFbsLayout { fbsElems, numElems };

    // vertex / index buffers
    std::vector<Byte> vertsData = Graphics::DownloadBuffer(m_vbh);
    const auto        fbsVerts  = builder.CreateVector(vertsData);

    std::vector<Byte> idxsData = Graphics::DownloadBuffer(m_ibh);
    const auto        fbsIdxs  = builder.CreateVector(idxsData);

    // aabb
    jmsh::AABB fbsAabb { ToJmshVec3(m_boundingBox.min), ToJmshVec3(m_boundingBox.max) };

    // mesh
    const auto fbsMesh = jmsh::CreateMesh(builder, fbsVerts, &pFbsLayout, fbsIdxs, &fbsAabb);
    builder.Finish(fbsMesh);

    // io
    return SaveBinaryToFile(_path, ConstMemView { builder.GetBufferPointer(), builder.GetSize() });
}

void Mesh::ComputeBoundingBox_(const ConstMemView _verts, const VertexLayout& _vl)
{
    // Position attribute required
    JAM_ASSERT(_vl.Has(eVertexAttribute::Position), "Position attribute is required to calculate bounding box");
    const VertexLayoutElement& posElement = _vl.GetElement(eVertexAttribute::Position);

    const Int32 byteWidth = Min(posElement.byteWidth, sizeof(Vec3));
    const Int32 offset    = posElement.offset;
    const Int32 stride    = _vl.GetStride();

    // vertex info
    const Int32 numVerts    = NarrowCast<Int32>(_verts.GetByteWidth() / stride);
    const Byte* pVertexData = _verts.GetDataPtr();

    Vec3 minPos { k_floatMax, k_floatMax, k_floatMax };
    Vec3 maxPos { k_floatMin, k_floatMin, k_floatMin };
    for (Int32 i = 0; i < numVerts; ++i)
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

    m_boundingBox = Aabb { minPos, maxPos };
}

Mesh::~Mesh()
{
    CleanUp_();
}

Mesh::Mesh(Mesh&& _other) noexcept
    : m_path(std::move(_other.m_path))
    , m_vbh(_other.m_vbh)
    , m_ibh(_other.m_ibh)
    , m_boundingBox(std::move(_other.m_boundingBox))
{
    _other.m_ibh         = k_invalidHandle;
    _other.m_vbh         = k_invalidHandle;
    _other.m_boundingBox = {};
    _other.m_path.clear();
}

Mesh& Mesh::operator=(Mesh&& _other) noexcept
{
    if (this != &_other)
    {
        CleanUp_();

        m_path        = std::move(_other.m_path);
        m_vbh         = _other.m_vbh;
        m_ibh         = _other.m_ibh;
        m_boundingBox = std::move(_other.m_boundingBox);

        _other.m_ibh         = k_invalidHandle;
        _other.m_vbh         = k_invalidHandle;
        _other.m_boundingBox = {};
        _other.m_path.clear();
    }
    return *this;
}

void Mesh::Unload()
{
    CleanUp_();
}

bool Mesh::IsValid() const
{
    return JAM_IS_VALID_HANDLE(m_vbh)
        && JAM_IS_VALID_HANDLE(m_ibh);
}

const fs::path& Mesh::GetPath() const
{
    return m_path;
}

eAssetType Mesh::GetAssetType() const
{
    return k_type;
}

UInt32 Mesh::GetTypeHash() const
{
    return k_hash;
}

std::string_view Mesh::GetTypeName() const
{
    return k_name;
}

UInt16 Mesh::GetSortKey() const
{
    return m_sortKey;
}

void Mesh::Submit() const
{
    Graphics::SetVertexBuffer(m_vbh);
    Graphics::SetIndexBuffer(m_ibh);
    Graphics::Submit();
}

VertexBufferHandle Mesh::GetVertexBuffer() const
{
    return m_vbh;
}

IndexBufferHandle Mesh::GetIndexBuffer() const
{
    return m_ibh;
}

const Aabb& Mesh::GetBoundingBox() const
{
    return m_boundingBox;
}

void Mesh::CleanUp_()
{
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
    m_boundingBox = {};
    m_path.clear();
}

}   // namespace jam