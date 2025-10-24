#pragma once
#include "GraphicsBase.h"
#include "IAsset.h"

namespace jam
{

class ImageTexture : public IAsset
{
public:
    ImageTexture() = default;
    ~ImageTexture() override;

    /// @brief 복사 금지
    ImageTexture(const ImageTexture& _other)            = delete;
    ImageTexture& operator=(const ImageTexture& _other) = delete;

    /// @brief 이동 허용
    ImageTexture(ImageTexture&& _other) noexcept;
    ImageTexture& operator=(ImageTexture&& _other) noexcept;

    /// @brief 경로로부터 텍스처를 로드합니다.
    eErrorCode Load(const fs::path& _path) override;

    /// @brief 텍스처를 언로드합니다.
    void Unload() override;

    /// @brief 텍스처를 저장합니다.
    /// @param _path 저장 경로
    /// @note .dds로 저장됩니다.
    /// _path의 확장자는 .dds로 변경됩니다.
    eErrorCode Save(const fs::path& _path) const override;

    JAM_NODISCARD bool  IsLoaded() const override;
    JAM_NODISCARD const fs::path& GetPath() const override;
    JAM_NODISCARD eAssetType      GetType() const override;
    JAM_NODISCARD UInt32          GetHash() const override;
    JAM_NODISCARD std::string_view GetName() const override;
    JAM_NODISCARD TextureHandle    GetHandle() const;

    /// @brief 텍스처 바인딩
    /// @param _shader 쉐이더 스테이지
    /// @param _slot 쉐이더 슬롯
    void Bind(eShader _shader, Int32 _slot) const;

    /// @brief 텍스처 바인딩 해제
    /// @param _shader 쉐이더 스테이지
    /// @param _slot 쉐이더 슬롯
    void Unbind(eShader _shader, Int32 _slot) const;

    static constexpr std::string_view k_name = NameOf<ImageTexture>();
    static constexpr UInt32           k_hash = HashOf<ImageTexture>();
    static constexpr eAssetType       k_type = eAssetType::ImageTexture;

private:
    void CleanUp_();

    fs::path      m_path   = {};
    TextureHandle m_handle = k_invalidHandle;
};

}   // namespace jam