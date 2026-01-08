#include "pch.h"

#include "Mesh.h"

#include "BaseUtilities.h"
#include "FileIOUtilities.h"
#include "Graphics.h"
#include "Hash.h"
#include "Profile.h"
#include "GeometryUtilities.h"

#include "resources/flatc/jmsh_generated.h"
namespace fbs = flatbuffers;

namespace jam
{

[[nodiscard]] static Vec3 Vec3From(
    const jmsh::Vec3 _vec)
{
    return { _vec.x(), _vec.y(), _vec.z() };
}

[[nodiscard]] static Aabb AabbFrom(
    const jmsh::AABB& _aabb)
{
    return { Vec3From(_aabb.min()), Vec3From(_aabb.max()) };
}

[[nodiscard]] static jmsh::Vec3 JmshVec3From(
    const Vec3 _vec)
{
    return { _vec.x, _vec.y, _vec.z };
}

[[nodiscard]] static jmsh::AABB JmshAabbFrom(
    const Aabb& _aabb)
{
    return { JmshVec3From(_aabb.min), JmshVec3From(_aabb.max) };
}

[[nodiscard]] static eVertexAttribute VertexAttributeFrom(
    const jmsh::eVertexAttribute _attrib)
{
    switch (_attrib)
    {
        case jmsh::eVertexAttribute_Position:
            return eVertexAttribute::Position;
        case jmsh::eVertexAttribute_Normal:
            return eVertexAttribute::Normal;
        case jmsh::eVertexAttribute_Tangent:
            return eVertexAttribute::Tangent;
        case jmsh::eVertexAttribute_Bitangent:
            return eVertexAttribute::Bitangent;
        case jmsh::eVertexAttribute_Color0:
            return eVertexAttribute::Color0;
        case jmsh::eVertexAttribute_Color1:
            return eVertexAttribute::Color1;
        case jmsh::eVertexAttribute_Color2:
            return eVertexAttribute::Color2;
        case jmsh::eVertexAttribute_Color3:
            return eVertexAttribute::Color3;
        case jmsh::eVertexAttribute_BlendIndices:
            return eVertexAttribute::BlendIndex;
        case jmsh::eVertexAttribute_BlendWeights:
            return eVertexAttribute::BlendWeight;
        case jmsh::eVertexAttribute_TexCoord0:
            return eVertexAttribute::TexCoord0;
        case jmsh::eVertexAttribute_TexCoord1:
            return eVertexAttribute::TexCoord1;
        case jmsh::eVertexAttribute_TexCoord2:
            return eVertexAttribute::TexCoord2;
        case jmsh::eVertexAttribute_TexCoord3:
            return eVertexAttribute::TexCoord3;
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
        case eVertexAttribute::Position:
            return jmsh::eVertexAttribute_Position;
        case eVertexAttribute::Normal:
            return jmsh::eVertexAttribute_Normal;
        case eVertexAttribute::Tangent:
            return jmsh::eVertexAttribute_Tangent;
        case eVertexAttribute::Bitangent:
            return jmsh::eVertexAttribute_Bitangent;
        case eVertexAttribute::Color0:
            return jmsh::eVertexAttribute_Color0;
        case eVertexAttribute::Color1:
            return jmsh::eVertexAttribute_Color1;
        case eVertexAttribute::Color2:
            return jmsh::eVertexAttribute_Color2;
        case eVertexAttribute::Color3:
            return jmsh::eVertexAttribute_Color3;
        case eVertexAttribute::BlendIndex:
            return jmsh::eVertexAttribute_BlendIndices;
        case eVertexAttribute::BlendWeight:
            return jmsh::eVertexAttribute_BlendWeights;
        case eVertexAttribute::TexCoord0:
            return jmsh::eVertexAttribute_TexCoord0;
        case eVertexAttribute::TexCoord1:
            return jmsh::eVertexAttribute_TexCoord1;
        case eVertexAttribute::TexCoord2:
            return jmsh::eVertexAttribute_TexCoord2;
        case eVertexAttribute::TexCoord3:
            return jmsh::eVertexAttribute_TexCoord3;
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
        case jmsh::eAttributeFormat_Float:
            return eAttributeFormat::Float;
        case jmsh::eAttributeFormat_Float2:
            return eAttributeFormat::Float2;
        case jmsh::eAttributeFormat_Float3:
            return eAttributeFormat::Float3;
        case jmsh::eAttributeFormat_Float4:
            return eAttributeFormat::Float4;
        case jmsh::eAttributeFormat_UNorm8x4:
            return eAttributeFormat::UNorm8x4;
        case jmsh::eAttributeFormat_UInt8x4:
            return eAttributeFormat::UInt8x4;
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
        case eAttributeFormat::Float:
            return jmsh::eAttributeFormat_Float;
        case eAttributeFormat::Float2:
            return jmsh::eAttributeFormat_Float2;
        case eAttributeFormat::Float3:
            return jmsh::eAttributeFormat_Float3;
        case eAttributeFormat::Float4:
            return jmsh::eAttributeFormat_Float4;
        case eAttributeFormat::UNorm8x4:
            return jmsh::eAttributeFormat_UNorm8x4;
        case eAttributeFormat::UInt8x4:
            return jmsh::eAttributeFormat_UInt8x4;
        default:
            JAM_ASSERT(0, "Unknown eAttributeFormat");
            return static_cast<jmsh::eAttributeFormat>(-1);
    }
}

}   // namespace jam

/////////////////////////////////////////////////////////////////////

namespace jam
{

bool Mesh::Load(
    const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    CleanUp_();

    // file io
    Vector<Byte> data = {};
    if (!MoveIfHasValue(LoadBinaryFile(_path), data))
    {
        GlobalBuffer::SetLastError("Failed to load material file: {}. {}", _path.native(), GlobalBuffer::GetLastError());
        return false;
    }

    // verify
    fbs::Verifier verifier = { data.data(), static_cast<size_t>(data.size()) };
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

    // name
    {
        const fbs::String* pJmshName = pJmshMesh->name();
        if (!pJmshName)
        {
            GlobalBuffer::SetLastError(R"(Failed to parse name in mesh file: "{}")", _path.native());
            return false;
        }

        m_name = pJmshName->str();
    }

    // vertex layout
    VertexLayout vl = {};
    {
        const jmsh::VertexLayout* pJmshVl = pJmshMesh->vertex_layout();
        if (!pJmshVl)
        {
            GlobalBuffer::SetLastError(R"(Failed to parse vertex vertexLayout in mesh file: "{}")", _path.native());
            return false;
        }

        // vertex layout
        const auto* pJmshVles   = pJmshVl->elems();
        const Int32 numJmshVles = pJmshVl->num_elems();
        if (!pJmshVles || numJmshVles == 0)
        {
            GlobalBuffer::SetLastError(R"(Vertex vertexLayout in mesh file is empty: "{}")", _path.native());
            return false;
        }

        // vertex layout
        for (Int32 i = 0; i < numJmshVles; ++i)
        {
            const jmsh::VertexLayoutElement* pVle = pJmshVles->Get(i);
            if (!pVle)
            {
                GlobalBuffer::SetLastError(R"(Failed to parse vertex vertexLayout element in mesh file: "{}")", _path.native());
                return false;
            }

            const eVertexAttribute attrib = VertexAttributeFrom(pVle->attrib());
            const eAttributeFormat format = AttributeFormatFrom(pVle->format());
            vl.Push(attrib, format);
        }
    }

    // vertex data
    const fbs::Vector<Byte>* pJmshVertData = pJmshMesh->vert_data();
    if (!pJmshVertData || pJmshVertData->size() == 0)
    {
        GlobalBuffer::SetLastError(R"(Vertex data in mesh file is empty: "{}")", _path.native());
        return false;
    }

    // index data
    const fbs::Vector<Byte>* pJmshIdxData = pJmshMesh->idx_data();
    if (!pJmshIdxData || pJmshIdxData->size() == 0)
    {
        GlobalBuffer::SetLastError(R"(Index data in mesh file is empty: "{}")", _path.native());
        return false;
    }

    // geometry data
    const Int32    numVerts = pJmshVertData->size() / vl.GetStride();
    const bool     bUseU32  = IsGreaterThanUInt16(numVerts);
    const CMemView vertData = { pJmshVertData->data(), NarrowCast<Int32>(pJmshVertData->size()) };
    const CMemView idxData  = { pJmshIdxData->data(), NarrowCast<Int32>(pJmshIdxData->size()) };

    // gpu resource creation
    m_path = _path;
    m_vbh  = Graphics::CreateVertexBuffer(vertData, vl);
    m_ibh  = Graphics::CreateIndexBuffer(idxData, bUseU32);

    // bounding box
    {
        const jmsh::AABB* pAabb = pJmshMesh->aabb();
        if (!pAabb)
        {
            GlobalBuffer::SetLastError(R"(Failed to parse AABB in mesh file: "{}")", _path.native());
            return false;
        }

        m_boundingBox = AabbFrom(*pAabb);
    }

    // update hash based on file path
    const UInt32 hash = HashOf(m_path.native());
    m_sortKey         = static_cast<UInt16>(hash);

    // mark
    m_bLoadedFromFile = true;
    return true;
}

void Mesh::LoadMemory(
    const CMemView      _verts,
    const VertexLayout& _vl,
    const CMemView      _idxData,
    const bool          _bUseU32)
{
    JAM_FUNCTION_PROFILE();
    CleanUp_();

    // gpu resource creation
    m_vbh = Graphics::CreateVertexBuffer(_verts, _vl);
    m_ibh = Graphics::CreateIndexBuffer(_idxData, _bUseU32);

    // compute bounding box
    ComputeBoundingBox_(_verts, _vl);

    // update sort key
    MurmurHash64 hasher {};
    hasher.Mix(_verts.GetPtr(), _verts.GetByteWidth());
    hasher.Mix(_idxData.GetPtr(), _idxData.GetByteWidth());
    hasher.Mix(_vl);
    hasher.Mix(_bUseU32);
    m_sortKey = static_cast<Int16>(hasher.Finish());

    // mark
    m_bLoadedFromFile = false;
}

void Mesh::LoadPlaceholder()
{
    JAM_FUNCTION_PROFILE();
    CleanUp_();

    m_vbh             = k_invalidHandle;
    m_ibh             = k_invalidHandle;
    m_sortKey         = MakeRandom<UInt16>(0, 0xFFFF);
    m_bLoadedFromFile = false;
}

bool Mesh::Save(
    const fs::path&      _path,
    const bool           _bReplaceExtension,
    const eSaveFileFlags _flags) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());

    JAM_ASSERT(IsValid(), "Mesh is not loaded");
    fbs::FlatBufferBuilder builder { 1024 };

    // name
    const fbs::Offset<fbs::String> jmshName = builder.CreateString(m_name);

    // vertex layout
    const VertexLayout&                   vl      = Graphics::GetVertexLayout(m_vbh);
    const Int32                           numVles = vl.GetNumElements();
    const Span<const VertexLayoutElement> vles    = vl.GetElements();

    jmsh::VertexLayoutElement jmshVles[NumEnums<eVertexAttribute>()] = {};
    for (Int32 i = 0; i < numVles; ++i)
    {
        const VertexLayoutElement&   vle        = vles[i];
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
    const jmsh::AABB jmshAabb = JmshAabbFrom(m_boundingBox);

    // mesh
    const auto jmshMesh = jmsh::CreateMesh(builder, jmshName, jmshVertData, &jmshVl, jmshIdxData, &jmshAabb);
    builder.Finish(jmshMesh);

    // make path
    if (_bReplaceExtension)
    {
        fs::path path = _path;
        path.replace_extension(k_jamMeshExt.GetPtr());
        return SaveBinaryFile(path, { builder.GetBufferPointer(), NarrowCast<Int32>(builder.GetSize()) }, _flags);
    }

    // io
    return SaveBinaryFile(_path, { builder.GetBufferPointer(), NarrowCast<Int32>(builder.GetSize()) }, _flags);
}

void Mesh::ComputeBoundingBox_(
    const CMemView      _vertData,
    const VertexLayout& _vl)
{
    JAM_ASSERT(_vl.Has(eVertexAttribute::Position), "Position attribute is required to calculate bounding box");

    // get position attribute info
    VertexReader vertReader { _vertData, _vl };
    const Int32  numVerts = CalcNumVertices(_vertData, _vl);

    m_boundingBox = {};
    for (Int32 i = 0; i < numVerts; ++i)
    {
        // read position
        Vec3 pos = {};
        vertReader.ReadAt(i, eVertexAttribute::Position, pos);

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
    , m_name(std::move(_other.m_name))
    , m_vbh(_other.m_vbh)
    , m_ibh(_other.m_ibh)
    , m_boundingBox(std::move(_other.m_boundingBox))
    , m_sortKey(_other.m_sortKey)
    , m_bLoadedFromFile(_other.m_bLoadedFromFile)
{
    _other.CleanUp_();
}

Mesh& Mesh::operator=(Mesh&& _other) noexcept
{
    if (this != &_other)
    {
        JAM_DESTROY_VERTEX_BUFFER(m_vbh);
        JAM_DESTROY_INDEX_BUFFER(m_ibh);

        m_path            = std::move(_other.m_path);
        m_vbh             = _other.m_vbh;
        m_ibh             = _other.m_ibh;
        m_boundingBox     = std::move(_other.m_boundingBox);
        m_sortKey         = _other.m_sortKey;
        m_name            = std::move(_other.m_name);
        m_bLoadedFromFile = _other.m_bLoadedFromFile;

        _other.CleanUp_();
    }
    return *this;
}

void Mesh::Unload()
{
    CleanUp_();
}

void Mesh::SetName(
    const StringView _name)
{
    m_name = _name.ToString();
}

void Mesh::SetVirtualPath(
    const fs::path& _path)
{
    JAM_ASSERT(m_bLoadedFromFile == false, "Cannot change path of a file-loaded asset");
    m_path = _path;
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

const StringView Mesh::GetName() const
{
    return m_name;
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

    m_path.clear();
    m_name        = {};
    m_boundingBox = {};
    m_sortKey     = 0;
}

}   // namespace jam