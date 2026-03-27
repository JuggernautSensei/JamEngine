#include "pch.h"

#include "Mesh.h"

#include "AssetSaveLoader.h"
#include "Graphics.h"
#include "MemoryHasher.h"
#include "Profile.h"
#include "GeometryUtilities.h"

namespace Jam
{

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
        StringViewDto   name     = {};
        VectorDto<Byte> vertData = {};
        VectorDto<Byte> idxData  = {};
        AabbDto         aabb     = {};
        VertexLayoutDto vl       = {};
    };

    using MeshLoader = AssetLoader<MeshDto>;
    using MeshSaver  = AssetSaver<MeshDto>;

}   // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

Ref<Mesh> Mesh::Load(
    const MemoryView    _vertData,
    const VertexLayout& _vl,
    const MemoryView    _idxData,
    const bool          _bUseU32)
{
    return new Mesh { _vertData, _vl, _idxData, _bUseU32 };
}

Mesh::Mesh(
    const MemoryView    _vertData,
    const VertexLayout& _vl,
    const MemoryView    _idxData,
    const bool          _bUseU32)
{
    // gpu resource creation
    m_vbh = GetGraphics().CreateVertexBuffer(_vertData, _vl);
    m_ibh = GetGraphics().CreateIndexBuffer(_idxData, _bUseU32);

    // compute bounding box
    ComputeBoundingBox_(_vertData, _vl);

    // sort key
    Murmur3Hasher64 hasher {};
    hasher.Mix(_vertData);
    hasher.Mix(_idxData);
    hasher.Mix(AsMemoryView(_vl));
    hasher.Mix(AsMemoryView(_bUseU32));
    const UInt64 hash = hasher.Finish();
    m_sortKey         = static_cast<UInt16>(hash);
    m_bLoadedFromFile = false;
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

Error Mesh::Load_(
    StringView _path)
{
    JAM_FUNCTION_MSG_PROFILE("path: {}", _path);

    // io
    Expect<AssetLoader<MeshDto>> loader = MeshLoader::Open(_path);
    if (!loader)
    {
        return loader.GetError();
    }

    // deserialize
    Expect<MeshDto> pDto = loader->Read();
    if (!pDto)
    {
        return pDto.GetError();
    }

    CleanUp_();
    m_name = StringFrom(pDto->name);

    // vertex layout
    VertexLayout vl = {};
    for (Int32 i = 0; i < pDto->vl.numElems; ++i)
    {
        const VertexLayoutElementDto& elemDto = pDto->vl.elems[i];
        vl.Push(elemDto.attrib, elemDto.format);
    }

    // gpu resource creation
    const bool bUseU32 = NeedU32Index(pDto->vertData, vl);
    m_vbh              = GetGraphics().CreateVertexBuffer(pDto->vertData, vl);
    m_ibh              = GetGraphics().CreateIndexBuffer(pDto->idxData, bUseU32);
    m_boundingBox      = pDto->aabb.ToAabb();

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

    StringView dir = {}, base = {}, ext = {};
    _path.Decompose(&dir, nullptr, &base, &ext);

    String path = dir;
    path.Append(base);
    path.Append(_bReplaceExtension ? k_extension : ext);

    Expect<MeshSaver> saver = MeshSaver::Open(path.ToStringView(), _policy);
    if (!saver)
    {
        return saver.GetError();
    }

    MeshDto dto  = {};
    dto.name     = StringViewDtoFrom(m_name);
    dto.vertData = VectorDtoFrom(GetGraphics().DownloadBuffer(m_vbh));
    dto.idxData  = VectorDtoFrom(GetGraphics().DownloadBuffer(m_ibh));
    dto.aabb     = AabbDto::FromAabb(m_boundingBox);

    // vertex layout
    const VertexLayout vl = GetGraphics().GetVertexLayout(m_vbh);
    dto.vl.numElems       = vl.GetNumElems();

    Int32 idx = 0;
    for (const VertexLayoutElement& elem: vl.GetElems())
    {
        VertexLayoutElementDto& elemDto = dto.vl.elems[idx++];
        elemDto.attrib                  = elem.attrib;
        elemDto.format                  = elem.format;
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
    return m_vbh && m_ibh;
}

StringView Mesh::GetPath() const
{
    return m_path;
}

eAssetType Mesh::GetAssetType() const
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

void Mesh::Submit() const
{
    Graphics& gfx = GetGraphics();

    gfx.SetVertexBuffer(m_vbh, 0);
    gfx.SetIndexBuffer(m_ibh);
    gfx.Submit();
}

void Mesh::CleanUp_()
{
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);

    m_path.Clear();
    m_name.Clear();
    m_boundingBox = {};
    m_sortKey     = 0;
}

void Mesh::ComputeBoundingBox_(
    const MemoryView    _vertData,
    const VertexLayout& _vl)
{
    JAM_ASSERT(_vl.Has(eVertexAttribute::Position), "Position attribute is required to calculate bounding box");

    // get position attribute info
    VertexReader reader { _vertData, _vl };
    const Int32  numVerts = CalcNumVertices(_vertData, _vl);

    // compute aabb
    m_boundingBox = {};
    for (Int32 i = 0; i < numVerts; ++i)
    {
        const Vec3 pos = reader.ReadAt<Vec3>(i, eVertexAttribute::Position);
        m_boundingBox.Merge(pos);   // merge aabb
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