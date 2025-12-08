#pragma once
#include "BoundingBox.h"
#include "GraphicsBase.h"
#include "IAsset.h"
#include "Magic.h"

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
    void LoadFromMemory(
        ConstMemView       _vertices,
        const VertexLayout& _layout,
        ConstMemView       _indices,
        eIndexFormat        _format      = eIndexFormat::U16,
        const fs::path&     _virtualPath = {});

    bool Save(const fs::path& _path) const override;
    void Unload() override;

    [[nodiscard]] bool             IsValid() const override;
    [[nodiscard]] const fs::path&  GetPath() const override;
    [[nodiscard]] eAssetType       GetAssetType() const override;
    [[nodiscard]] UInt32           GetTypeHash() const override;
    [[nodiscard]] std::string_view GetTypeName() const override;
    [[nodiscard]] UInt16           GetSortKey() const;

    [[nodiscard]] VertexBufferHandle GetVertexBuffer() const;
    [[nodiscard]] IndexBufferHandle  GetIndexBuffer() const;
    [[nodiscard]] const Aabb&        GetBoundingBox() const;

    void Submit() const;

    static constexpr UInt32           k_hash      = HashOf<Mesh>();
    static constexpr std::string_view k_name      = NameOf<Mesh>();
    static constexpr eAssetType       k_type      = eAssetType::Mesh;
    static constexpr std::string_view k_extension = k_jamMeshExt;

private:
    void CleanUp_();
    void ComputeBoundingBox_(
        ConstMemView       _verts,
        const VertexLayout& _vl);

    fs::path           m_path        = {};
    VertexBufferHandle m_vbh         = k_invalidHandle;
    IndexBufferHandle  m_ibh         = k_invalidHandle;
    Aabb               m_boundingBox = {};
    UInt16             m_sortKey     = 0;
};

}   // namespace jam
