#include "pch.h"

#include "Mesh.h"

#include "AssetSaveLoader.h"
#include "Graphics.h"
#include "MemoryHasher.h"
#include "Profile.h"
#include "GeometryUtilities.h"

namespace Jam
{

VertexLayout Mesh::s_positionOnlyVl = [] {
    VertexLayout layout = {};
    layout.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
    return layout;
}();

namespace Dto
{
    struct VertexLayoutElement
    {
        eVertexAttribute attrib = eVertexAttribute::Position;
        eAttributeFormat format = eAttributeFormat::Float;
    };

    struct VertexLayout
    {
        Array<VertexLayoutElement, CountOf<eVertexAttribute>()> elems    = {};
        Int32                                                   numElems = 0;
    };

    struct Mesh
    {
        StringView   name         = {};
        Vector<Vec3> positions    = {};
        Vector<Byte> attribData   = {};
        VertexLayout attribLayout = {};
        Aabb         boundingBox  = {};
        Vector<Byte> idxData      = {};
    };

}   // namespace Dto

namespace
{

    using MeshLoader = AssetLoader<Dto::Mesh>;
    using MeshSaver  = AssetSaver<Dto::Mesh>;

}   // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

Mesh::~Mesh()
{
    CleanUp_();
}

Ref<Mesh> Mesh::Load(
    const Span<const Vec3> _position,
    const MemoryView       _attribData,
    const VertexLayout&    _attribLayout,
    const MemoryView       _idxData)
{
    return new Mesh(_position, _attribData, _attribLayout, _idxData);
}

Ref<Mesh> Mesh::Load(
    const Span<const Vec3> _position,
    const MemoryView       _idxData)
{
    return new Mesh(_position, _idxData);
}

Expect<Ref<Mesh>> Mesh::LoadFromFile(
    const StringView _path)
{
    Ref<Mesh> pMesh = new Mesh;
    if (const Error err = pMesh->LoadFromFile_(_path))
    {
        return err;
    }
    return pMesh;
}

Mesh::Mesh(
    const Span<const Vec3> _positions,
    const MemoryView       _attribData,
    const VertexLayout&    _attribLayout,
    const MemoryView       _idxData)
{
    JAM_ASSERT(!_positions.empty(), "Position data cannot be empty.");
    JAM_ASSERT(!_idxData.IsEmpty(), "Index data cannot be empty.");

    // vertex buffer
    Graphics& gfx = GetGraphics();
    m_positionVbh = gfx.CreateVertexBuffer(_positions, s_positionOnlyVl);
    m_attribVbh   = gfx.CreateVertexBuffer(_attribData, _attribLayout);

    // index buffer
    const bool bUseU32 = std::cmp_greater(_positions.size(), Limits<UInt16>::kHighest);
    m_ibh              = gfx.CreateIndexBuffer(_idxData, bUseU32);

    // compute bounding box
    m_boundingBox = {};
    for (const Vec3& pos: _positions)
    {
        m_boundingBox.Merge(pos);
    }

    // sort key
    Murmur3Hasher64 hasher {};
    hasher.Mix(_positions);
    hasher.Mix(_attribData);
    hasher.Mix(_idxData);
    hasher.Mix(AsMemoryView(_attribLayout.GetHash()));
    const UInt64 hash = hasher.Finish();
    m_sortKey         = static_cast<UInt16>(hash);
}

Mesh::Mesh(
    const Span<const Vec3> _positios,
    const MemoryView       _idxData)
{
    // vertex buffer
    Graphics& gfx = GetGraphics();
    m_positionVbh = gfx.CreateVertexBuffer(_positios, s_positionOnlyVl);
    m_attribVbh   = kInvalidHandle;

    // index buffer
    const bool bUseU32 = std::cmp_greater(_positios.size(), Limits<UInt16>::kHighest);
    m_ibh              = gfx.CreateIndexBuffer(_idxData, bUseU32);

    // compute bounding box
    m_boundingBox = {};
    for (const Vec3& pos: _positios)
    {
        m_boundingBox.Merge(pos);
    }

    // sort key
    Murmur3Hasher64 hasher {};
    hasher.Mix(_positios);
    hasher.Mix(_idxData);
    const UInt64 hash = hasher.Finish();
    m_sortKey         = static_cast<UInt16>(hash);
}

Error Mesh::LoadFromFile_(
    const StringView _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);

    // io
    Expect<AssetLoader<Dto::Mesh>> loader = MeshLoader::Open(_path);
    if (!loader)
    {
        return loader.GetError();
    }

    // deserialize
    Expect<Dto::Mesh> meshOrNot = loader->Read();
    if (!meshOrNot)
    {
        return meshOrNot.GetError();
    }

    CleanUp_();

    const Dto::Mesh& mesh = meshOrNot.GetValue();
    m_name                = Dto::Unwrap(mesh.name);

    // gpu resource creation
    Graphics&  gfx     = GetGraphics();
    const bool bUseU32 = mesh.positions.size() > Limits<UInt16>::kHighest;
    m_positionVbh      = gfx.CreateVertexBuffer(mesh.positions, s_positionOnlyVl);
    m_ibh              = gfx.CreateIndexBuffer(mesh.idxData, bUseU32);
    m_boundingBox      = Unwrap(mesh.boundingBox);

    // attrib
    if (!meshOrNot->attribData.empty())
    {
        const Dto::VertexLayout& layout = mesh.attribLayout;

        VertexLayout vl = {};
        for (Int32 i = 0; i < layout.numElems; ++i)
        {
            const Dto::VertexLayoutElement& elem = layout.elems[i];
            vl.Push(elem.attrib, elem.format);
        }

        m_attribVbh = gfx.CreateVertexBuffer(meshOrNot->attribData, vl);
    }

    // finalize
    const StringView path = _path;
    const UInt32     hash = path.Hash32();
    m_sortKey             = static_cast<UInt16>(hash);
    m_path                = _path;
    return Error::None;
}

Error Mesh::Save(
    const StringView                 _path,
    const bool                       _bReplaceExtension,
    const eFileWriterCollisionPolicy _policy) const
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);
    JAM_ASSERT(IsValid(), "Cannot save an invalid mesh");

    String path = _path;
    if (_bReplaceExtension)
    {
        path.ReplaceExtension(kExtension);
    }

    Expect<MeshSaver> saver = MeshSaver::Open(path.ToStringView(), _policy);
    if (!saver)
    {
        return saver.GetError();
    }

    Graphics&                 gfx       = GetGraphics();
    const VertexBufferInfo&   vbInfo    = gfx.GetVertexBufferInfo(m_positionVbh);
    const IndexBufferInfo&    ibInfo    = gfx.GetIndexBufferInfo(m_ibh);
    const Int32               byteWidth = Max(vbInfo.byteWidth, ibInfo.byteWidth);
    const StorageBufferHandle readback  = gfx.CreateReadbackBuffer(byteWidth);

    Dto::Mesh mesh   = {};
    mesh.name        = Dto::Wrap(m_name);
    mesh.boundingBox = Dto::Wrap(m_boundingBox);

    // vertex data
    gfx.CopyBuffer(readback, 0, m_positionVbh, 0, vbInfo.byteWidth);
    mesh.positions.resize(vbInfo.byteWidth);
    gfx.ReadBuffer(readback, mesh.positions);

    // index data
    gfx.CopyBuffer(readback, 0, m_ibh, 0, ibInfo.byteWidth);
    mesh.idxData.resize(ibInfo.byteWidth);
    gfx.ReadBuffer(readback, mesh.idxData);

    if (m_attribVbh)
    {
        // vertex layout
        const VertexLayout vl      = gfx.GetVertexLayout(m_attribVbh);
        mesh.attribLayout.numElems = vl.GetNumElems();

        Int32 idx = 0;
        for (const VertexLayoutElement& e: vl.GetElems())
        {
            Dto::VertexLayoutElement& elem = mesh.attribLayout.elems[idx++];
            elem.format                    = e.format;
            elem.attrib                    = e.attrib;
        }

        // attrib data
        const VertexBufferInfo& attribVbInfo = gfx.GetVertexBufferInfo(m_attribVbh);
        gfx.CopyBuffer(readback, 0, m_attribVbh, 0, attribVbInfo.byteWidth);
        mesh.attribData.resize(attribVbInfo.byteWidth);
        gfx.ReadBuffer(readback, mesh.attribData);
    }

    return saver->Write(mesh);
}

void Mesh::SetName(
    const StringView _name)
{
    m_name = _name;
}

void Mesh::Invalidate()
{
    CleanUp_();
}

bool Mesh::IsValid() const
{
    return m_positionVbh && m_ibh;
}

StringView Mesh::GetPath() const
{
    return m_path;
}

eAsset Mesh::GetAssetType() const
{
    return kAssetType;
}

UInt32 Mesh::GetTypeHash() const
{
    return kTypeHash;
}

StringView Mesh::GetTypeName() const
{
    return kTypeName;
}

UInt16 Mesh::GetSortKey() const
{
    return m_sortKey;
}

StringView Mesh::GetName() const
{
    return m_name;
}

StringView Mesh::GetExtension() const
{
    return kExtension;
}

StringView Mesh::GetFilter() const
{
    return kFilter;
}

VertexBufferHandle Mesh::GetVertexBuffer() const
{
    return m_positionVbh;
}

IndexBufferHandle Mesh::GetIndexBuffer() const
{
    return m_ibh;
}

const Aabb& Mesh::GetBoundingBox() const
{
    return m_boundingBox;
}

void Mesh::Submit(
    const bool _bSubmitPositionOnly) const
{
    Graphics& gfx = GetGraphics();
    if (!_bSubmitPositionOnly && m_attribVbh)
    {
        VertexBufferBinding bindings[] = {
            { m_positionVbh, 0 },
            {   m_attribVbh, 0 }
        };

        gfx.SetVertexBuffers(bindings);
        gfx.SetIndexBuffer(m_ibh);
        gfx.Submit();
    }
    else
    {
        gfx.SetVertexBuffer(m_positionVbh);
        gfx.SetIndexBuffer(m_ibh);
        gfx.Submit();
    }
}

void Mesh::Submit(
    const VertexBufferHandle _instanceVbh,
    const Int32              _instanceOffset,
    const Int32              _numInstances,
    const bool               _bSubmitPositionOnly) const
{
    Graphics& gfx = GetGraphics();
    if (!_bSubmitPositionOnly && m_attribVbh)
    {
        VertexBufferBinding bindings[] = {
            { m_positionVbh, 0 },
            {   m_attribVbh, 0 }
        };

        gfx.SetVertexBuffers(bindings, kUseAll, _instanceVbh, _instanceOffset, _numInstances);
        gfx.SetIndexBuffer(m_ibh);
        gfx.Submit();
    }
    else
    {
        gfx.SetVertexBuffer(m_positionVbh, 0, kUseAll, _instanceVbh, _instanceOffset, _numInstances);
        gfx.SetIndexBuffer(m_ibh);
        gfx.Submit();
    }
}

void Mesh::CleanUp_()
{
    JAM_DESTROY_VERTEX_BUFFER(m_positionVbh);
    JAM_DESTROY_VERTEX_BUFFER(m_attribVbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);

    m_path.Clear();
    m_name.Clear();
    m_boundingBox = {};
    m_sortKey     = 0;
}

void WriteToArchive(
    ArchiveWriter&   _archive,
    const Ref<Mesh>& _pMesh)
{
    if (_pMesh && _pMesh->IsValid())
    {
        _archive.Write(_pMesh->GetPath());
    }
}

Error ReadFromArchive(
    ArchiveReader& _archive,
    Ref<Mesh>&     _pMesh)
{
    StringView path = {};
    if (const Error err = _archive.Read(path))
    {
        return err;
    }

    Expect<Ref<Mesh>> pMesh = Mesh::LoadFromFile(path);
    if (!pMesh)
    {
        return pMesh.GetError();
    }

    _pMesh = pMesh.Take();
    return Error::None;
}

}   // namespace Jam