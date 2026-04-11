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

namespace
{
    struct VertexLayoutElementDto
    {
        eVertexAttribute attrib = eVertexAttribute::Position;
        eAttributeFormat format = eAttributeFormat::Float;
    };

    struct VertexLayoutDto
    {
        ArrayDto<VertexLayoutElementDto, CountOf<eVertexAttribute>()> elems    = {};
        Int32                                                         numElems = 0;
    };

    struct MeshDto
    {
        StringViewDto      name         = {};
        VectorDto<Vec3Dto> position     = {};
        VectorDto<Byte>    attribData   = {};
        VertexLayoutDto    attribLayout = {};
        AabbDto            aabb         = {};
        VectorDto<Byte>    idxData      = {};
    };

    using MeshLoader = AssetLoader<MeshDto>;
    using MeshSaver  = AssetSaver<MeshDto>;

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
    const MemoryView       _idxData,
    const bool             _bUse32)
{
    return new Mesh(_position, _attribData, _attribLayout, _idxData, _bUse32);
}

Ref<Mesh> Mesh::Load(
    const Span<const Vec3> _position,
    const MemoryView       _idxData,
    const bool             _bUse32)
{
    return new Mesh(_position, _idxData, _bUse32);
}

Expect<Ref<Mesh>> Mesh::LoadFromFile(
    const StringView _path)
{
    Ref<Mesh> pMesh = new Mesh;
    if (const Error err = pMesh->Load_(_path))
    {
        return err;
    }
    return pMesh;
}

Mesh::Mesh(
    const Span<const Vec3> _positions,
    const MemoryView       _attribData,
    const VertexLayout&    _attribLayout,
    const MemoryView       _idxData,
    const bool             _bUseU32)
{
    JAM_ASSERT(!_positions.empty(), "Position data cannot be empty.");
    JAM_ASSERT(!_idxData.IsEmpty(), "Index data cannot be empty.");

    // gpu resource
    Graphics& gfx = GetGraphics();
    m_positionVbh = gfx.CreateVertexBuffer(_positions, s_positionOnlyVl);
    m_attribVbh   = gfx.CreateVertexBuffer(_attribData, _attribLayout);
    m_ibh         = gfx.CreateIndexBuffer(_idxData, _bUseU32);

    // compute bounding box
    ComputeBoundingBox_(_positions);

    // sort key
    Murmur3Hasher64 hasher {};
    hasher.Mix(_positions);
    hasher.Mix(_attribData);
    hasher.Mix(_idxData);
    hasher.Mix(AsMemoryView(_attribLayout.GetHash()));
    hasher.Mix(AsMemoryView(_bUseU32));
    const UInt64 hash = hasher.Finish();
    m_sortKey         = static_cast<UInt16>(hash);
    m_bLoadedFromFile = false;
}

Mesh::Mesh(
    const Span<const Vec3> _positios,
    const MemoryView       _idxData,
    const bool             _bUseU32)
{
    // gpu resource
    Graphics& gfx = GetGraphics();
    m_positionVbh = gfx.CreateVertexBuffer(_positios, s_positionOnlyVl);
    m_attribVbh   = k_nullHandle;
    m_ibh         = gfx.CreateIndexBuffer(_idxData, _bUseU32);

    // compute bounding box
    ComputeBoundingBox_(_positios);

    // sort key
    Murmur3Hasher64 hasher {};
    hasher.Mix(_positios);
    hasher.Mix(_idxData);
    hasher.Mix(AsMemoryView(_bUseU32));
    const UInt64 hash = hasher.Finish();
    m_sortKey         = static_cast<UInt16>(hash);
    m_bLoadedFromFile = false;
}

Error Mesh::Load_(
    const StringView _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);

    // io
    Expect<AssetLoader<MeshDto>> loader = MeshLoader::Open(_path);
    if (!loader)
    {
        return loader.GetError();
    }

    // deserialize
    Expect<MeshDto> dto = loader->Read();
    if (!dto)
    {
        return dto.GetError();
    }

    CleanUp_();
    m_name = StringFrom(dto->name);

    // gpu resource creation
    Graphics&  gfx     = GetGraphics();
    const bool bUseU32 = dto->position.size() > Limits<UInt16>::k_highest;
    m_positionVbh      = gfx.CreateVertexBuffer(dto->position, s_positionOnlyVl);
    m_ibh              = gfx.CreateIndexBuffer(dto->idxData, bUseU32);
    m_boundingBox      = dto->aabb.ToAabb();

    // attrib
    if (!dto->attribData.empty())
    {
        VertexLayout vl = {};
        for (Int32 i = 0; i < dto->attribLayout.numElems; ++i)
        {
            const VertexLayoutElementDto& elemDto = dto->attribLayout.elems[i];
            vl.Push(elemDto.attrib, elemDto.format);
        }

        m_attribVbh = gfx.CreateVertexBuffer(dto->attribData, vl);
    }

    // finalize
    const StringView path = _path;
    const UInt32     hash = path.Hash32();
    m_sortKey             = static_cast<UInt16>(hash);
    m_path                = _path;
    m_bLoadedFromFile     = true;
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
        path.ReplaceExtension(k_extension);
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

    MeshDto dto = {};
    dto.name    = StringViewDtoFrom(m_name);
    dto.aabb    = AabbDto::FromAabb(m_boundingBox);

    // vertex data
    gfx.CopyBuffer(readback, 0, m_positionVbh, 0, vbInfo.byteWidth);
    dto.position.resize(vbInfo.byteWidth);
    gfx.ReadBuffer(readback, dto.position);

    // index data
    gfx.CopyBuffer(readback, 0, m_ibh, 0, ibInfo.byteWidth);
    dto.idxData.resize(ibInfo.byteWidth);
    gfx.ReadBuffer(readback, dto.idxData);

    if (m_attribVbh)
    {
        // vertex layout
        const VertexLayout vl     = gfx.GetVertexLayout(m_attribVbh);
        dto.attribLayout.numElems = vl.GetNumElems();

        Int32 idx = 0;
        for (const VertexLayoutElement& elem: vl.GetElems())
        {
            VertexLayoutElementDto& elemDto = dto.attribLayout.elems[idx++];
            elemDto.attrib                  = elem.attrib;
            elemDto.format                  = elem.format;
        }

        // attrib data
        const VertexBufferInfo& attribVbInfo = gfx.GetVertexBufferInfo(m_attribVbh);
        gfx.CopyBuffer(readback, 0, m_attribVbh, 0, attribVbInfo.byteWidth);
        dto.attribData.resize(attribVbInfo.byteWidth);
        gfx.ReadBuffer(readback, dto.attribData);
    }

    return saver->Write(dto);
}

void Mesh::SetName(
    const StringView _name)
{
    m_name = _name;
}

void Mesh::SetVirtualPath(
    const StringView _path)
{
    JAM_ASSERT(m_bLoadedFromFile == false, "Cannot change path of a file-loaded asset");
    m_path = _path;
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
    return k_assetType;
}

UInt32 Mesh::GetTypeHash() const
{
    return k_typeHash;
}

StringView Mesh::GetTypeName() const
{
    return k_typeName;
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
    return k_extension;
}

StringView Mesh::GetFilter() const
{
    return k_filter;
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

void Mesh::Submit() const
{
    Graphics& gfx = GetGraphics();

    if (m_attribVbh)
    {
        gfx.SetVertexBuffer(m_positionVbh, 0);
        gfx.SetVertexBuffer(m_attribVbh, 1);
        gfx.SetIndexBuffer(m_ibh);
        gfx.Submit();
    }
    else
    {
        gfx.SetVertexBuffer(m_positionVbh, 0);
        gfx.SetIndexBuffer(m_ibh);
        gfx.Submit();
    }
}

void Mesh::SubmitPositionOnly() const
{
    Graphics& gfx = GetGraphics();
    gfx.SetVertexBuffer(m_positionVbh, 0);
    gfx.SetIndexBuffer(m_ibh);
    gfx.Submit();
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

void Mesh::ComputeBoundingBox_(
    const Span<const Vec3> _positions)
{
    JAM_ASSERT(m_positionVbh, "Position vertex buffer must be valid to compute bounding box.");

    // compute aabb
    m_boundingBox = {};
    for (const Vec3& pos: _positions)
    {
        m_boundingBox.Merge(pos);
    }
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