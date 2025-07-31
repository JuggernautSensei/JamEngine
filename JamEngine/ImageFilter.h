#pragma once
#include "ShaderProgram.h"
#include "Textures.h"

namespace jam
{

class ImageFilter
{
public:
    ImageFilter()                                  = default;
    virtual ~ImageFilter()                         = default;
    ImageFilter(const ImageFilter&)                = default;
    ImageFilter& operator=(const ImageFilter&)     = default;
    ImageFilter(ImageFilter&&) noexcept            = default;
    ImageFilter& operator=(ImageFilter&&) noexcept = default;

    virtual void             Bind(const Texture2D& _inputTexture);
    NODISCARD virtual UInt32 GetHash() const = 0;

    void                       SetOutputTexture(const Texture2D& _texture) { m_outputTexture = _texture; }
    NODISCARD const Texture2D& GetOutputTexture() const { return m_outputTexture; }

protected:
    void InitializeFilterFrame_(UInt32 _width, UInt32 _height, DXGI_FORMAT _format);

    Texture2D m_outputTexture;
};

class BlurDownFilter : public ImageFilter
{
public:
    BlurDownFilter()                                     = default;
    ~BlurDownFilter() override                           = default;
    BlurDownFilter(const BlurDownFilter&)                = default;
    BlurDownFilter& operator=(const BlurDownFilter&)     = default;
    BlurDownFilter(BlurDownFilter&&) noexcept            = default;
    BlurDownFilter& operator=(BlurDownFilter&&) noexcept = default;

    void             Initialize(UInt32 _width, UInt32 _height, DXGI_FORMAT _format);
    void             Bind(const Texture2D& _inputTexture) override;
    NODISCARD UInt32 GetHash() const override { return HashOf<BlurDownFilter>(); }
};

class BlurUpFilter : public ImageFilter
{
public:
    BlurUpFilter()                                   = default;
    ~BlurUpFilter() override                         = default;
    BlurUpFilter(const BlurUpFilter&)                = default;
    BlurUpFilter& operator=(const BlurUpFilter&)     = default;
    BlurUpFilter(BlurUpFilter&&) noexcept            = default;
    BlurUpFilter& operator=(BlurUpFilter&&) noexcept = default;

    void             Initialize(UInt32 _width, UInt32 _height, DXGI_FORMAT _format);
    void             Bind(const Texture2D& _inputTexture) override;
    NODISCARD UInt32 GetHash() const override { return HashOf<BlurUpFilter>(); }
};

class CombineFilter : public ImageFilter
{
public:
    CombineFilter()                                    = default;
    ~CombineFilter() override                          = default;
    CombineFilter(const CombineFilter&)                = default;
    CombineFilter& operator=(const CombineFilter&)     = default;
    CombineFilter(CombineFilter&&) noexcept            = default;
    CombineFilter& operator=(CombineFilter&&) noexcept = default;

    void             Initialize(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, const Texture2D& _conbineDestinationTexture);
    void             Bind(const Texture2D& _inputTexture) override;
    NODISCARD UInt32 GetHash() const override { return HashOf<CombineFilter>(); }

private:
    Texture2D m_combineDestinationTexture;
};

class FogFilter : public ImageFilter
{
public:
    FogFilter()                                = default;
    ~FogFilter() override                      = default;
    FogFilter(const FogFilter&)                = default;
    FogFilter& operator=(const FogFilter&)     = default;
    FogFilter(FogFilter&&) noexcept            = default;
    FogFilter& operator=(FogFilter&&) noexcept = default;

    void             Initialize(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, const Texture2D& _depthTexture);
    void             Bind(const Texture2D& _inputTexture) override;
    NODISCARD UInt32 GetHash() const override { return HashOf<FogFilter>(); }

private:
    Texture2D m_depthTexture;
};

enum class eFXAAQuality
{
    VeryLow,
    Low,
    Medium,
    High,
    VeryHigh,
    Ultra,
};

class FXAAFilter : public ImageFilter
{
public:
    FXAAFilter()                                 = default;
    ~FXAAFilter() override                       = default;
    FXAAFilter(const FXAAFilter&)                = default;
    FXAAFilter& operator=(const FXAAFilter&)     = default;
    FXAAFilter(FXAAFilter&&) noexcept            = default;
    FXAAFilter& operator=(FXAAFilter&&) noexcept = default;

    void             Initialize(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, eFXAAQuality _quality);
    void             Bind(const Texture2D& _inputTexture) override;
    NODISCARD UInt32 GetHash() const override { return HashOf<FXAAFilter>(); }

private:
    ShaderProgram m_shader;
};

enum class eToneMappingFilterType
{
    Uncharted2,
    Reinhard,
    WhitePreservingReinhard,
    LumaBasedReinhard,
    RombDaHouse,
    Filmic,
    Linear,
};

class ToneMappingFilter : public ImageFilter
{
public:
    ToneMappingFilter()                                        = default;
    ~ToneMappingFilter() override                              = default;
    ToneMappingFilter(const ToneMappingFilter&)                = default;
    ToneMappingFilter& operator=(const ToneMappingFilter&)     = default;
    ToneMappingFilter(ToneMappingFilter&&) noexcept            = default;
    ToneMappingFilter& operator=(ToneMappingFilter&&) noexcept = default;

    void             Initialize(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, eToneMappingFilterType _type);
    void             Bind(const Texture2D& _inputTexture) override;
    NODISCARD UInt32 GetHash() const override { return HashOf<ToneMappingFilter>(); }

private:
    ShaderProgram m_shader;
};

class SamplingFilter : public ImageFilter
{
public:
    SamplingFilter()                                     = default;
    ~SamplingFilter() override                           = default;
    SamplingFilter(const SamplingFilter&)                = default;
    SamplingFilter& operator=(const SamplingFilter&)     = default;
    SamplingFilter(SamplingFilter&&) noexcept            = default;
    SamplingFilter& operator=(SamplingFilter&&) noexcept = default;

    void             Initialize(UInt32 _width, UInt32 _height, DXGI_FORMAT _format);
    void             Bind(const Texture2D& _inputTexture) override;
    NODISCARD UInt32 GetHash() const override { return HashOf<SamplingFilter>(); }
};

}   // namespace jam