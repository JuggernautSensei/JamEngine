#pragma once
#include "BoundingVolume.h"
#include "GraphicsBase.h"
#include "IAsset.h"
#include "Magics.h"

namespace jam
{

class Mesh : public IAsset
{
public:
    Mesh() = default;
    ~Mesh() override;

    Mesh(const Mesh& _other)            = delete;
    Mesh& operator=(const Mesh& _other) = delete;
    Mesh(Mesh&& _other) noexcept;
    Mesh& operator=(Mesh&& _other) noexcept;

    bool Load(const fs::path& _path) override;
    void LoadMemory(CMemView _verts, const VertexLayout& _vl, CMemView _idxData, bool _bUseU32 = false);
    void LoadPlaceholder();   // this cannot be used to render, only for placeholder

    bool Save(const fs::path& _path, bool _bReplaceExtension = true, eSaveFileFlags _flags = eSaveFileFlags_None) const override;
    void Unload() override;
    void SetName(StringView _name) override;
    void SetVirtualPath(const fs::path& _path) override;

    [[nodiscard]] bool             IsValid() const override;
    [[nodiscard]] const fs::path&  GetPath() const override;
    [[nodiscard]] eAssetType       GetAssetType() const override;
    [[nodiscard]] UInt32           GetTypeHash() const override;
    [[nodiscard]] StringView       GetTypeName() const override;
    [[nodiscard]] UInt16           GetSortKey() const;
    [[nodiscard]] const StringView GetName() const override;

    [[nodiscard]] VertexBufferHandle GetVertexBuffer() const;
    [[nodiscard]] IndexBufferHandle  GetIndexBuffer() const;
    [[nodiscard]] const Aabb&        GetBoundingBox() const;

    void Submit() const;

    static constexpr UInt32     k_hash = HashOf<Mesh>();
    static constexpr StringView k_name = NameOf<Mesh>();
    static constexpr eAssetType k_type = eAssetType::Mesh;

private:
    void CleanUp_();
    void ComputeBoundingBox_(CMemView _vertData, const VertexLayout& _vl);

    fs::path           m_path            = {};
    std::string        m_name            = {};
    VertexBufferHandle m_vbh             = k_invalidHandle;
    IndexBufferHandle  m_ibh             = k_invalidHandle;
    Aabb               m_boundingBox     = {};
    UInt16             m_sortKey         = 0;
    bool               m_bLoadedFromFile = false;
};

}   // namespace jam
