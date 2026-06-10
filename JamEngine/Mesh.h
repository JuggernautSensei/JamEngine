#pragma once
#include "File.h"
#include "IAsset.h"
#include "Archive.h"
#include "GraphicsBase.h"
#include "BoundingVolume.h"

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

    [[nodiscard]] static Ref<Mesh> Load(Span<const Vec3> _position, MemoryView _attribData, const VertexLayout& _attribLayout, MemoryView _indexData);
    [[nodiscard]] static Ref<Mesh> Load(Span<const Vec3> _position, MemoryView _indexData);
    [[nodiscard]] static Ref<Mesh> Load(Span<const Vec2> _position, MemoryView _attribData, const VertexLayout& _attribLayout, MemoryView _indexData);
    [[nodiscard]] static Ref<Mesh> Load(Span<const Vec2> _position, MemoryView _indexData);

    [[nodiscard]] static Expect<Ref<Mesh>> LoadFromFile(StringView _path);

    Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const override;
    void  SetName(StringView _name) override;
    void  Invalidate() override;

    [[nodiscard]] bool       IsValid() const override;
    [[nodiscard]] StringView GetPath() const override;
    [[nodiscard]] eAsset     GetAssetType() const override;
    [[nodiscard]] UInt64     GetTypeHash() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt16     GetSortKey() const;
    [[nodiscard]] StringView GetName() const override;
    [[nodiscard]] StringView GetExtension() const override;
    [[nodiscard]] StringView GetFilter() const override;

    [[nodiscard]] VertexBufferHandle GetVertexBuffer() const;
    [[nodiscard]] IndexBufferHandle  GetIndexBuffer() const;
    [[nodiscard]] const Aabb&        GetBoundingBox() const;

    void Submit(bool _bSubmitPositionOnly = false) const;
    void Submit(VertexBufferHandle _instanceVbh, Int32 _instanceOffset = 0, Int32 _numInstances = kAll, bool _bSubmitPositionOnly = false) const;

    static constexpr StringView kTypeName  = NameOf<Mesh>();
    static constexpr UInt64     kTypeHash  = HashOf<Mesh>();
    static constexpr eAsset     kAssetType = eAsset::Mesh;
    static constexpr StringView kExtension = ".jmsh";
    static constexpr StringView kFilter    = "*.jmsh";

private:
    Mesh() = default;
    Mesh(Span<const Vec3> _positions, MemoryView _attribData, const VertexLayout& _attribLayout, MemoryView _indexData);
    Mesh(Span<const Vec3> _positios, MemoryView _indexData);
    Mesh(Span<const Vec2> _positions, MemoryView _attribData, const VertexLayout& _attribLayout, MemoryView _indexData);
    Mesh(Span<const Vec2> _positions, MemoryView _indexData);

    Error LoadFromFile_(StringView _path) override;
    void  CleanUp_();

    String m_path        = {};
    String m_name        = {};
    UInt16 m_sortKey     = 0;
    Aabb   m_boundingBox = {};

    VertexBufferHandle m_posVbh         = kInvalidHandle;
    Int32              m_posVbByteWidth = 0;

    VertexBufferHandle m_attribVbh         = kInvalidHandle;
    Int32              m_attribVbByteWidth = 0;

    IndexBufferHandle m_ibh         = kInvalidHandle;
    Int32             m_ibByteWidth = 0;

    static VertexLayout s_posFloat3Layout;
    static VertexLayout s_posFloat2Layout;
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Mesh>& _pMesh);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Mesh>& _pMesh);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Mesh>);

}   // namespace Jam
