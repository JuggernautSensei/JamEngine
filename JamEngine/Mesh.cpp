#include "pch.h"

#include "Mesh.h"

#include "BaseUtilities.h"
#include "Cista.h"
#include "FileIOUtilities.h"
#include "Graphics.h"
#include "Hash.h"
#include "Profile.h"
#include "GeometryUtilities.h"

namespace jam
{

struct VertexLayoutElementSchema
{
    eVertexAttribute attrib = eVertexAttribute::Position;
    eAttributeFormat format = eAttributeFormat::Float;
};

struct MeshSchema
{
    CistaString       name       = {};
    CistaVector<Byte> vertexData = {};
    CistaVector<Byte> indexData  = {};
    Aabb              aabb       = {};

    Array<VertexLayoutElementSchema, NumEnums<eVertexAttribute>()> vles    = {};
    Int32                                                          numVles = 0;
};

////////////////////////////////////////////////////////////////////

bool Mesh::Load(
    const fs::path& _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path.native());
    CleanUp_();

    // io
    Vector<Byte> data = {};
    if (!MoveIfHasValue(LoadBinaryFile(_path), data))
    {
        return false;
    }

    // deserialize
    MeshSchema* const pMesh = nullptr;
    if (!MoveIfHasValue(DeserializeCista<MeshSchema>(data), pMesh))
    {
        return false;
    }

    // name
    m_name = pMesh->name.str();

    // vertex layout
    VertexLayout vl = {};
    for (Int32 i = 0; i < pMesh->numVles; ++i)
    {
        const VertexLayoutElementSchema& vleSchema = pMesh->vles[i];
        vl.Push(vleSchema.attrib, vleSchema.format);
    }

    // gpu resource creation
    {
        const CMemView vertData { pMesh->vertexData.data(), NarrowCast<Int32>(pMesh->vertexData.size()) };
        const CMemView idxData { pMesh->indexData.data(), NarrowCast<Int32>(pMesh->indexData.size()) };
        const Int32    numVerts = CalcNumVertices(vertData, vl);
        const bool     bUseU32  = IsGreaterThanUInt16(numVerts);

        m_vbh = Graphics::CreateVertexBuffer(vertData, vl);
        m_ibh = Graphics::CreateIndexBuffer(idxData, bUseU32);
    }

    // aabb
    m_boundingBox = pMesh->aabb;

    // update sort key
    const UInt32 hash = HashOf(_path.native());
    m_sortKey         = static_cast<Int16>(hash);

    // path
    m_path = _path;

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
    if (!IsValid())
    {
        GlobalBuffer::SetLastError("Cannot save invalid mesh to file: {}", _path.native());
        return false;
    }

    // prepare schema
    MeshSchema schema = {};
    schema.name       = CistaString { m_name };
    schema.vertexData = cista::offset::to_vec(Graphics::DownloadBuffer(m_vbh));
    schema.indexData  = cista::offset::to_vec(Graphics::DownloadBuffer(m_ibh));
    schema.aabb       = m_boundingBox;

    // vertex layout
    {
        const VertexLayout vl = Graphics::GetVertexLayout(m_vbh);
        schema.numVles        = vl.GetNumElements();

        Int32 idx = 0;
        for (const VertexLayoutElement& vle: vl.GetElements())
        {
            VertexLayoutElementSchema& vleSchema = schema.vles[idx++];
            vleSchema.attrib                     = vle.attrib;
            vleSchema.format                     = vle.format;
        }
    }

    // serialize
    Vector<Byte> data = {};
    if (!MoveIfHasValue(SerializeCista(schema), data))
    {
        return false;
    }

    // io
    return SaveBinaryFile(_path, data, _flags);
}

void Mesh::ComputeBoundingBox_(
    const CMemView      _vertData,
    const VertexLayout& _vl)
{
    JAM_ASSERT(_vl.Has(eVertexAttribute::Position), "Position attribute is required to calculate bounding box");

    // get position attribute info
    VertexReader vertReader { _vertData, _vl };
    const Int32  numVerts = CalcNumVertices(_vertData, _vl);

    // compute aabb
    m_boundingBox = {};
    for (Int32 i = 0; i < numVerts; ++i)
    {
        Vec3 pos = {};
        vertReader.ReadAt(i, eVertexAttribute::Position, pos);   // read position
        m_boundingBox.Merge(pos);                                // merge aabb
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