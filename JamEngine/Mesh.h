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

    Mesh(const Mesh& _other)            = delete;
    Mesh& operator=(const Mesh& _other) = delete;
    Mesh(Mesh&& _other) noexcept;
    Mesh& operator=(Mesh&& _other) noexcept;

    [[nodiscard]] static Ref<Mesh>         Load(MemoryView _vertData, const VertexLayout& _vl, MemoryView _idxData, bool _bUseU32 = false);
    [[nodiscard]] static Expect<Ref<Mesh>> LoadFromFile(StringView _path);

    Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const override;
    void  SetName(StringView _name) override;
    void  SetVirtualPath(StringView _path) override;
    void  Invalidate() override;

    [[nodiscard]] bool       IsValid() const override;
    [[nodiscard]] StringView GetPath() const override;
    [[nodiscard]] eAssetType GetAssetType() const override;
    [[nodiscard]] UInt32     GetTypeHash() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt16     GetSortKey() const;
    [[nodiscard]] StringView GetName() const override;
    [[nodiscard]] StringView GetExtension() const override;
    [[nodiscard]] StringView GetFilter() const override;

    [[nodiscard]] VertexBufferHandle GetVertexBuffer() const;
    [[nodiscard]] IndexBufferHandle  GetIndexBuffer() const;
    [[nodiscard]] const Aabb&        GetBoundingBox() const;

    void Submit() const;

    static constexpr StringView k_typeName  = NameOf<Mesh>();
    static constexpr UInt32     k_typeHash  = HashOf<Mesh>();
    static constexpr eAssetType k_assetType = eAssetType::Mesh;
    static constexpr StringView k_extension = ".jmsh";
    static constexpr StringView k_filter    = "*.jmsh";

private:
    Mesh() = default;
    Mesh(MemoryView _vertData, const VertexLayout& _vl, MemoryView _idxData, bool _bUseU32 = false);
    Error Load_(StringView _path) override;
    void  CleanUp_();
    void  ComputeBoundingBox_(MemoryView _vertData, const VertexLayout& _vl);

    String             m_path            = {};
    String             m_name            = {};
    VertexBufferHandle m_vbh             = k_nullHandle;
    IndexBufferHandle  m_ibh             = k_nullHandle;
    Aabb               m_boundingBox     = {};
    UInt16             m_sortKey         = 0;
    bool               m_bLoadedFromFile = false;
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Mesh>& _pMesh);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Mesh>& _pMesh);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Mesh>);

}   // namespace Jam
