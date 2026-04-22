#pragma once
#include "BoundingVolume.h"
#include "File.h"
#include "GraphicsBase.h"
#include "IAsset.h"

namespace Jam
{

class Mesh : public IAsset
{
public:
    ~Mesh() override;

    Mesh(const Mesh& _other)                = delete;
    Mesh& operator=(const Mesh& _other)     = delete;
    Mesh(Mesh&& _other) noexcept            = delete;
    Mesh& operator=(Mesh&& _other) noexcept = delete;

    [[nodiscard]] static Ref<Mesh>         Load(Span<const Vec3> _position, MemoryView _attribData, const VertexLayout& _attribLayout, MemoryView _idxData);
    [[nodiscard]] static Ref<Mesh>         Load(Span<const Vec3> _position, MemoryView _idxData);
    [[nodiscard]] static Expect<Ref<Mesh>> LoadFromFile(StringView _path);

    Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const override;
    void  SetName(StringView _name) override;
    void  Invalidate() override;

    [[nodiscard]] bool       IsValid() const override;
    [[nodiscard]] StringView GetPath() const override;
    [[nodiscard]] eAsset     GetAssetType() const override;
    [[nodiscard]] UInt32     GetTypeHash() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt16     GetSortKey() const;
    [[nodiscard]] StringView GetName() const override;
    [[nodiscard]] StringView GetExtension() const override;
    [[nodiscard]] StringView GetFilter() const override;

    [[nodiscard]] VertexBufferHandle GetVertexBuffer() const;
    [[nodiscard]] IndexBufferHandle  GetIndexBuffer() const;
    [[nodiscard]] const Aabb&        GetBoundingBox() const;

    void Submit(bool _bSubmitPositionOnly = false) const;
    void Submit(VertexBufferHandle _instanceVbh, Int32 _instanceOffset = 0, Int32 _numInstances = kUseAll, bool _bSubmitPositionOnly = false) const;

    static constexpr StringView kTypeName  = NameOf<Mesh>();
    static constexpr UInt32     kTypeHash  = HashOf<Mesh>();
    static constexpr eAsset     kAssetType = eAsset::Mesh;
    static constexpr StringView kExtension = ".jmsh";
    static constexpr StringView kFilter    = "*.jmsh";

private:
    Mesh() = default;
    Mesh(Span<const Vec3> _positions, MemoryView _attribData, const VertexLayout& _attribLayout, MemoryView _idxData);
    Mesh(Span<const Vec3> _positios, MemoryView _idxData);

    Error LoadFromFile_(StringView _path) override;
    void  CleanUp_();

    String m_path    = {};
    String m_name    = {};
    UInt16 m_sortKey = 0;

    VertexBufferHandle m_positionVbh = kInvalidHandle;
    VertexBufferHandle m_attribVbh   = kInvalidHandle;
    IndexBufferHandle  m_ibh         = kInvalidHandle;
    Aabb               m_boundingBox = {};

    static VertexLayout s_positionOnlyVl;
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Mesh>& _pMesh);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Mesh>& _pMesh);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Mesh>);

}   // namespace Jam
