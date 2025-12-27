#include "pch.h"

#include "Mesh.h"

#include "BaseUtilities.h"
#include "FileIOUtilities.h"
#include "Graphics.h"
#include "Hash.h"
#include "Profile.h"
#include "resources/flatc/jmsh_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace jam
{

[[nodiscard]] static Vec3 Vec3From(
    const jmsh::Vec3 _vec)
{
    return { _vec.x(), _vec.y(), _vec.z() };
}

[[nodiscard]] static jmsh::Vec3 JmshVec3From(
    const Vec3 _vec)
{
    return jmsh::Vec3 { _vec.x, _vec.y, _vec.z };
}

[[nodiscard]] static eVertexAttribute VertexAttributeFrom(
    const jmsh::eVertexAttribute _attrib)
{
    switch (_attrib)
    {
        case jmsh::eVertexAttribute_Position: return eVertexAttribute::Position;
        case jmsh::eVertexAttribute_Normal: return eVertexAttribute::Normal;
        case jmsh::eVertexAttribute_Tangent: return eVertexAttribute::Tangent;
        case jmsh::eVertexAttribute_Bitangent: return eVertexAttribute::Bitangent;
        case jmsh::eVertexAttribute_Color0: return eVertexAttribute::Color0;
        case jmsh::eVertexAttribute_Color1: return eVertexAttribute::Color1;
        case jmsh::eVertexAttribute_Color2: return eVertexAttribute::Color2;
        case jmsh::eVertexAttribute_Color3: return eVertexAttribute::Color3;
        case jmsh::eVertexAttribute_BlendIndices: return eVertexAttribute::BlendIndex;
        case jmsh::eVertexAttribute_BlendWeights: return eVertexAttribute::BlendWeight;
        case jmsh::eVertexAttribute_TexCoord0: return eVertexAttribute::TexCoord0;
        case jmsh::eVertexAttribute_TexCoord1: return eVertexAttribute::TexCoord1;
        case jmsh::eVertexAttribute_TexCoord2: return eVertexAttribute::TexCoord2;
        case jmsh::eVertexAttribute_TexCoord3: return eVertexAttribute::TexCoord3;
        default:
            JAM_ASSERT(0, "Unknown jmsh::eVertexAttribute");
            return static_cast<eVertexAttribute>(-1);
    }
}

[[nodiscard]] static jmsh::eVertexAttribute JmshVertexAttributeFrom(
    const eVertexAttribute _attrib)
{
    switch (_attrib)
    {
        case eVertexAttribute::Position: return jmsh::eVertexAttribute_Position;
        case eVertexAttribute::Normal: return jmsh::eVertexAttribute_Normal;
        case eVertexAttribute::Tangent: return jmsh::eVertexAttribute_Tangent;
        case eVertexAttribute::Bitangent: return jmsh::eVertexAttribute_Bitangent;
        case eVertexAttribute::Color0: return jmsh::eVertexAttribute_Color0;
        case eVertexAttribute::Color1: return jmsh::eVertexAttribute_Color1;
        case eVertexAttribute::Color2: return jmsh::eVertexAttribute_Color2;
        case eVertexAttribute::Color3: return jmsh::eVertexAttribute_Color3;
        case eVertexAttribute::BlendIndex: return jmsh::eVertexAttribute_BlendIndices;
        case eVertexAttribute::BlendWeight: return jmsh::eVertexAttribute_BlendWeights;
        case eVertexAttribute::TexCoord0: return jmsh::eVertexAttribute_TexCoord0;
        case eVertexAttribute::TexCoord1: return jmsh::eVertexAttribute_TexCoord1;
        case eVertexAttribute::TexCoord2: return jmsh::eVertexAttribute_TexCoord2;
        case eVertexAttribute::TexCoord3: return jmsh::eVertexAttribute_TexCoord3;
        default:
            JAM_ASSERT(0, "Unknown eVertexAttribute");
            return static_cast<jmsh::eVertexAttribute>(-1);
    }
}

[[nodiscard]] static eAttributeFormat AttributeFormatFrom(
    const jmsh::eAttributeFormat _format)
{
    switch (_format)
    {
        case jmsh::eAttributeFormat_Float: return eAttributeFormat::Float;
        case jmsh::eAttributeFormat_Float2: return eAttributeFormat::Float2;
        case jmsh::eAttributeFormat_Float3: return eAttributeFormat::Float3;
        case jmsh::eAttributeFormat_Float4: return eAttributeFormat::Float4;
        case jmsh::eAttributeFormat_UNorm8x4: return eAttributeFormat::UNorm8x4;
        case jmsh::eAttributeFormat_UInt8x4: return eAttributeFormat::UInt8x4;
        default:
            JAM_ASSERT(0, "Unknown jmsh::eAttributeFormat");
            return static_cast<eAttributeFormat>(-1);
    }
}

[[nodiscard]] static jmsh::eAttributeFormat JmshAttributeFormatFrom(
    const eAttributeFormat _format)
{
    switch (_format)
    {
        case eAttributeFormat::Float: return jmsh::eAttributeFormat_Float;
        case eAttributeFormat::Float2: return jmsh::eAttributeFormat_Float2;
        case eAttributeFormat::Float3: return jmsh::eAttributeFormat_Float3;
        case eAttributeFormat::Float4: return jmsh::eAttributeFormat_Float4;
        case eAttributeFormat::UNorm8x4: return jmsh::eAttributeFormat_UNorm8x4;
        case eAttributeFormat::UInt8x4: return jmsh::eAttributeFormat_UInt8x4;
        default:
            JAM_ASSERT(0, "Unknown eAttributeFormat");
            return static_cast<jmsh::eAttributeFormat>(-1);
    }
}

}   // namespace jam

namespace jam
{

bool Mesh::Load(const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    CleanUp_();

    // file io
    const Optional<Vector<Byte>> optData = LoadBinaryFile(_path);
    if (!optData)
    {
        return false;
    }
    const Vector<Byte>& data = optData.value();

    // verify
    flatbuffers::Verifier verifier = { data.data(), data.size() };
    if (!jmsh::VerifyMeshBuffer(verifier))
    {
        GlobalBuffer::SetLastError(R"(Mesh file is corrupted or invalid: "{}")", _path.native());
        return false;
    }

    // parse
    const jmsh::Mesh* pJmshMesh = jmsh::GetMesh(data.data());
    if (!pJmshMesh)
    {
        GlobalBuffer::SetLastError(R"(Failed to parse mesh file: "{}")", _path.native());
        return false;
    }

    // vertex vertexLayout
    const jmsh::VertexLayout* pJmshVl = pJmshMesh->vertex_layout();
    if (!pJmshVl)
    {
        GlobalBuffer::SetLastError(R"(Failed to parse vertex vertexLayout in mesh file: "{}")", _path.native());
        return false;
    }

    // vertex vertexLayout element parse
    const auto* pJmshVles   = pJmshVl->elems();
    const Int32 numJmshVles = pJmshVl->count();
    if (!pJmshVles || numJmshVles == 0)
    {
        GlobalBuffer::SetLastError(R"(Vertex vertexLayout in mesh file is empty: "{}")", _path.native());
        return false;
    }

    // build vertex vertexLayout
    VertexLayout vl;
    for (Int32 i = 0; i < numJmshVles; ++i)
    {
        if (const jmsh::VertexLayoutElement* pVle = pJmshVles->Get(i))
        {
            const eVertexAttribute attrib = VertexAttributeFrom(pVle->attrib());
            const eAttributeFormat format = AttributeFormatFrom(pVle->format());
            vl.Push(attrib, format);
        }
        else
        {
            GlobalBuffer::SetLastError(R"(Failed to parse vertex vertexLayout element in mesh file: "{}")", _path.native());
            return false;
        }
    }

    // vertex data
    const flatbuffers::Vector<Byte>* pJmshVertData = pJmshMesh->vertices();
    if (!pJmshVertData || pJmshVertData->size() == 0)
    {
        GlobalBuffer::SetLastError(R"(Vertex data in mesh file is empty: "{}")", _path.native());
        return false;
    }

    // index data
    const flatbuffers::Vector<Byte>* pJmshIdxData = pJmshMesh->indices();
    if (!pJmshIdxData || pJmshIdxData->size() == 0)
    {
        GlobalBuffer::SetLastError(R"(Index data in mesh file is empty: "{}")", _path.native());
        return false;
    }

    // geometry data
    const Int32    numVerts = NarrowCast<Int32>(pJmshVertData->size() / vl.GetStride());
    const bool     bUseU32  = numVerts > 0xFFFF;
    const CMemView vertData = { pJmshVertData->data(), NarrowCast<Int32>(pJmshVertData->size()) };
    const CMemView idxData  = { pJmshIdxData->data(), NarrowCast<Int32>(pJmshIdxData->size()) };

    // gpu resource creation
    m_path = _path;
    m_vbh  = Graphics::CreateVertexBuffer(vertData, vl);
    m_ibh  = Graphics::CreateIndexBuffer(idxData, bUseU32);

    // bounding box
    if (const jmsh::AABB* pAabb = pJmshMesh->aabb())
    {
        m_boundingBox.min = Vec3From(pAabb->min());
        m_boundingBox.max = Vec3From(pAabb->max());
    }
    else
    {
        // compute if empty
        ComputeBoundingBox_(vertData, vl);
    }

    // update hash based on file path
    const UInt32 hash = HashOf(m_path.native());
    m_sortKey         = static_cast<UInt16>(hash);
    return true;
}

void Mesh::LoadMemory(
    const CMemView      _verts,
    const VertexLayout& _vl,
    const CMemView      _idxData,
    const bool          _bUseU32,
    const fs::path&     _virtualPath)
{
    JAM_FUNCTION_PROFILE();
    CleanUp_();

    // gpu resource creation
    m_path = _virtualPath;
    m_vbh  = Graphics::CreateVertexBuffer(_verts, _vl);
    m_ibh  = Graphics::CreateIndexBuffer(_idxData, _bUseU32);

    // compute bounding box
    ComputeBoundingBox_(_verts, _vl);

    // update sort key
    if (m_path.empty())
    {
        MurmurHash64 hasher;
        hasher.Begin();
        hasher.Add(_verts.GetPtr(), _verts.GetByteWidth());
        hasher.Add(&_vl, sizeof(VertexLayout));
        hasher.Add(_idxData.GetPtr(), _idxData.GetByteWidth());
        hasher.Add(&_bUseU32, sizeof(bool));

        const UInt64 hash = hasher.End();
        m_sortKey         = static_cast<UInt16>(hash);
    }
    else
    {
        const UInt32 hash = HashOf(m_path.native());
        m_sortKey         = static_cast<UInt16>(hash);
    }
}

bool Mesh::Save(const fs::path& _path) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());

    JAM_ASSERT(IsValid(), "Mesh is not loaded");
    flatbuffers::FlatBufferBuilder builder { 1024 };

    // vertex layout
    const VertexLayout&       vl           = Graphics::GetVertexLayout(m_vbh);
    const Int32               numVles      = vl.GetNumElements();
    jmsh::VertexLayoutElement jmshVles[14] = {};
    for (Int32 i = 0; i < numVles; ++i)
    {
        const VertexLayoutElement&   vle        = vl.GetElements()[i];
        const jmsh::eVertexAttribute jmshAttrib = JmshVertexAttributeFrom(vle.attrib);
        const jmsh::eAttributeFormat jmshFormat = JmshAttributeFormatFrom(vle.format);
        const Int16                  offset     = NarrowCast<Int16>(vle.offset);
        jmshVles[i]                             = jmsh::VertexLayoutElement { jmshAttrib, jmshFormat, offset };
    }
    const jmsh::VertexLayout jmshVl = { jmshVles, numVles };

    // fbs geometry data
    Vector<Byte> vertData     = Graphics::DownloadBuffer(m_vbh);
    const auto   jmshVertData = builder.CreateVector(vertData.data(), vertData.size());
    Vector<Byte> idxData      = Graphics::DownloadBuffer(m_ibh);
    const auto   jmshIdxData  = builder.CreateVector(idxData.data(), idxData.size());

    // aabb
    const jmsh::AABB jmshAabb = { JmshVec3From(m_boundingBox.min), JmshVec3From(m_boundingBox.max) };

    // mesh
    const auto jmshMesh = jmsh::CreateMesh(builder, jmshVertData, &jmshVl, jmshIdxData, &jmshAabb);
    builder.Finish(jmshMesh);

    // io
    return SaveBinaryFile(_path, { builder.GetBufferPointer(), NarrowCast<Int32>(builder.GetSize()) });
}

void Mesh::ComputeBoundingBox_(const CMemView _vertData, const VertexLayout& _vl)
{
    JAM_ASSERT(_vl.Has(eVertexAttribute::Position), "Position attribute is required to calculate bounding box");

    // position attribute info
    const VertexLayoutElement& vle       = _vl.GetElement(eVertexAttribute::Position);
    const Int32                byteWidth = Min(vle.byteWidth, sizeof(Vec3));   // assume float3
    const Int32                offset    = vle.offset;
    const Int32                stride    = _vl.GetStride();

    // vertex info
    const Int32 numVerts  = CalcNumVertices(_vertData, _vl);
    const Byte* pVertData = _vertData.GetPtr();

    m_boundingBox = {};
    for (Int32 i = 0; i < numVerts; ++i)
    {
        // get position (assume float3)
        const Byte* pSrcData = pVertData + i * stride + offset;
        Vec3        pos      = Vec3::Zero;
        std::memcpy(&pos, pSrcData, byteWidth);

        // merge
        m_boundingBox.Merge(pos);
    }
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

StringView Mesh::GetTypeName() const
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