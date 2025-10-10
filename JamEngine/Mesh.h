#pragma once
#include "GfxBase.h"
#include "IAsset.h"

// clang-format off
namespace jam { class MemoryView; }
// clang-format on

namespace jam
{

////////////////////////////////////
///          MESH ASSET          ///
////////////////////////////////////

/// @brief 메쉬 클래스. 현재는 삼각형 메시만 지원.
/// 필요시 확장가능하나 현재는 염두해두지 않음.
class Mesh : public IAsset
{
public:
    /// @brief 메쉬 기본 생성자
    Mesh() = default;
    ~Mesh() override;

    /// @brief 복사 금지
    Mesh(const Mesh& _other)            = delete;
    Mesh& operator=(const Mesh& _other) = delete;

    /// @brief 이동 허용
    Mesh(Mesh&& _other) noexcept;
    Mesh& operator=(Mesh&& _other) noexcept;

    /// @brief 경로로부터 메쉬를 로드합니다.
    /// @param _path 로드할 파일 경로
    /// @return 로드 성공 여부
    eErrorCode Load(const fs::path& _path) override;

    /// @brief 버텍스와 인덱스 데이터를 사용하여 메쉬를 로드합니다.
    /// @param _vertices 버텍스 데이터
    /// @param _layout 버텍스 레이아웃
    /// @param _indices 인덱스 데이터
    /// @param _format 인덱스 포맷
    /// @note 내부적으로 버텍스 버퍼와 인덱스 버퍼를 생성합니다.
    void Load(MemoryView   _vertices,
              const VertexLayout& _layout,
              MemoryView   _indices,
              eIndexFormat        _format = eIndexFormat::U16);

    /// @brief 메쉬를 저장합니다.
    /// @param _path 저장할 파일 경로
    /// @return 저장 성공 여부
    /// @note 확장자는 .jmsh로 저장됩니다.
    eErrorCode Save(const fs::path& _path) const override;

    void Unload() override;

    JAM_NODISCARD bool  IsLoaded() const override;
    JAM_NODISCARD const fs::path& GetPath() const override;
    JAM_NODISCARD eAssetType      GetType() const override;
    JAM_NODISCARD UInt32          GetHash() const override;
    JAM_NODISCARD std::string_view GetName() const override;

    /// @brief 바인딩 및 제출
    void Submit() const;

    /// @brief 교차 여부 반환
    JAM_NODISCARD bool Intersect(
        const Mat4&    _meshTransform,
        const Frustum& _frustumInWorld) const;

    /// @brief 교차 여부 반환
    JAM_NODISCARD bool Intersect(
        const Mat4& _meshTransform,
        const AABB& _aabbInWorld) const;

    /// @brief 교차 여부 반환
    JAM_NODISCARD bool Intersect(
        const Mat4&   _meshTransform,
        const Sphere& _sphereInWorld) const;

    /// @brief 교차 여부 반환
    JAM_NODISCARD bool Intersect(
        const Mat4& _meshTransform,
        const OBB&  _obbInWorld) const;

    /// @brief 교차 여부와 교차 지점까지의 거리를 반환
    /// @note 교차하지 않을 경우 bool = false, float = 0.0f 반환
    JAM_NODISCARD IntersectResult Intersect(
        const Mat4& _meshTransform,
        const Ray&  _rayInWorld) const;

    /// @brief 버텍스 버퍼 핸들 반환
    JAM_NODISCARD VertexBufferHandle GetVertexBuffer() const;

    /// @brief 인덱스 버퍼 핸들 반환
    JAM_NODISCARD IndexBufferHandle GetIndexBuffer() const;

    /// @brief 바운딩 박스 반환 *** 로컬 공간 ***
    JAM_NODISCARD const AABB& GetBoundingBox() const;

    static constexpr UInt32           k_hash      = HashOf<Mesh>();
    static constexpr std::string_view k_name      = NameOf<Mesh>();
    static constexpr eAssetType       k_type      = eAssetType::Mesh;
    static constexpr std::string_view k_extension = ".jmsh";
    static constexpr Version          k_version { 1, 0, 0 };

private:
    void CleanUp_();
    void CalcBoundingBox_(MemoryView _vertices, const VertexLayout& _layout);

    fs::path           m_path               = {};
    VertexBufferHandle m_vertexBufferHandle = k_invalidHandle;
    IndexBufferHandle  m_indexBufferHandle  = k_invalidHandle;
    AABB               m_boundingBox        = {};
};

using MeshHandle = Handle<Mesh>;

}   // namespace jam
