#pragma once
#include "ImageFilter.h"
#include "Viewport.h"

namespace jam
{

class Texture2D;
class ShaderProgram;

class PostProcess
{
public:
    void Initialize(std::span<std::shared_ptr<ImageFilter>> _filters);
    void Render(const Texture2D& _inputTexture) const;

private:
    std::vector<std::shared_ptr<ImageFilter>> m_filters;
};

class PostProcessBuilder
{
public:
    PostProcessBuilder& AddSamplingFilter(UInt32 _width, UInt32 _height, DXGI_FORMAT _format);
    PostProcessBuilder& AddFXAAFilter(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, eFXAAQuality _quality);
    PostProcessBuilder& AddToneMappingFilter(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, eToneMappingFilterType _type);
    PostProcessBuilder& AddBloomFilter(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, UInt32 _bloomLevel, const Texture2D& _bloomDestinationTexture);
    PostProcessBuilder& AddFogFilter(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, const Texture2D& _depthTexture);

    NODISCARD PostProcess Build(const Texture2D& _outputTexture) const;

private:
    enum eFilterFlags_ : Int32
    {
        eFilterFlags_None        = 0,
        eFilterFlags_FXAA        = 1 << 0,
        eFilterFlags_ToneMapping = 1 << 1,
        eFilterFlags_Bloom       = 1 << 2,
        eFilterFlags_Fog         = 1 << 3,
        eFilterFlags_Sampling    = 1 << 4,
    };
    using eFilterFlags         = Int32;
    eFilterFlags m_filterFlags = eFilterFlags_None;

    // sampling
    UInt32      m_samplingWidth  = 0;
    UInt32      m_samplingHeight = 0;
    DXGI_FORMAT m_samplingFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    // fxaa
    eFXAAQuality m_fxaaQuality = eFXAAQuality::High;
    UInt32       m_fxaaWidth   = 0;
    UInt32       m_fxaaHeight  = 0;
    DXGI_FORMAT  m_fxaaFormat  = DXGI_FORMAT_R16G16B16A16_FLOAT;

    // tone mapping
    eToneMappingFilterType m_toneMappingType   = eToneMappingFilterType::Linear;
    UInt32                 m_toneMappingWidth  = 0;
    UInt32                 m_toneMappingHeight = 0;
    DXGI_FORMAT            m_toneMappingFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    // bloom
    UInt32      m_bloomLevel = 1;
    UInt32      m_bloomWidth  = 0;
    UInt32      m_bloomHeight = 0;
    DXGI_FORMAT m_bloomFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    // fog
    Texture2D   m_depthTexture;
    UInt32      m_fogWidth  = 0;
    UInt32      m_fogHeight = 0;
    DXGI_FORMAT m_fogFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
};

}   // namespace jam