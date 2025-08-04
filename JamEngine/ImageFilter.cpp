#include "pch.h"

#include "ImageFilter.h"

#include "ShaderCollection.h"
#include "Renderer.h"
#include "ShaderBridge.h"

namespace jam
{

void ImageFilter::Bind(const Texture2D& _inputTexture)
{
    _inputTexture.BindAsShaderResource(eShader::PixelShader, k_postProcessInputTexture1Slot);
    m_outputTexture.BindAsRenderTarget();
}

void ImageFilter::InitializeFilterFrame_(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format)
{
    m_outputTexture.Initialize(_width, _height, _format, eResourceAccess::GPUWriteable, eViewFlags_ShaderResource | eViewFlags_RenderTarget);
    m_outputTexture.AttachSRV(_format);
    m_outputTexture.AttachRTV(_format);
}

void BlurDownFilter::Initialize(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format)
{
    InitializeFilterFrame_(_width, _height, _format);
}

void BlurDownFilter::Bind(const Texture2D& _inputTexture)
{
    ImageFilter::Bind(_inputTexture);
    ShaderCollection::BloomDownFilterShader().Bind();
}

void BlurUpFilter::Initialize(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format)
{
    InitializeFilterFrame_(_width, _height, _format);
}

void BlurUpFilter::Bind(const Texture2D& _inputTexture)
{
    ImageFilter::Bind(_inputTexture);
    ShaderCollection::BloomUpFilterShader().Bind();
}

void CombineFilter::Initialize(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const Texture2D& _conbineDestinationTexture)
{
    InitializeFilterFrame_(_width, _height, _format);
    m_combineDestinationTexture = _conbineDestinationTexture;
}

void CombineFilter::Bind(const Texture2D& _inputTexture)
{
    ImageFilter::Bind(_inputTexture);
    m_combineDestinationTexture.BindAsShaderResource(eShader::PixelShader, k_postProcessInputTexture2Slot);
    ShaderCollection::BloomCombineFilterShader().Bind();
}

void FogFilter::Initialize(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const Texture2D& _depthTexture)
{
    InitializeFilterFrame_(_width, _height, _format);
    m_depthTexture = _depthTexture;
}

void FogFilter::Bind(const Texture2D& _inputTexture)
{
    ImageFilter::Bind(_inputTexture);
    m_depthTexture.BindAsShaderResource(eShader::PixelShader, k_postProcessInputTexture2Slot);
    ShaderCollection::FogFilterShader().Bind();
}

void FXAAFilter::Initialize(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const eFXAAQuality _quality)
{
    InitializeFilterFrame_(_width, _height, _format);

    switch (_quality)
    {
        case eFXAAQuality::VeryLow:
            m_shader = ShaderCollection::FXAAFilterQuality0Shader();
            break;

        case eFXAAQuality::Low:
            m_shader = ShaderCollection::FXAAFilterQuality1Shader();
            break;

        case eFXAAQuality::Medium:
            m_shader = ShaderCollection::FXAAFilterQuality2Shader();
            break;

        case eFXAAQuality::High:
            m_shader = ShaderCollection::FXAAFilterQuality3Shader();
            break;

        case eFXAAQuality::VeryHigh:
            m_shader = ShaderCollection::FXAAFilterQuality4Shader();
            break;

        case eFXAAQuality::Ultra:
            m_shader = ShaderCollection::FXAAFilterQuality5Shader();
            break;

        default:
            JAM_ASSERT(false, "Unknown FXAA quality level");
            break;
    }
}

void FXAAFilter::Bind(const Texture2D& _inputTexture)
{
    ImageFilter::Bind(_inputTexture);
    m_shader.Bind();
}

void ToneMappingFilter::Initialize(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const eToneMappingFilterType _type)
{
    InitializeFilterFrame_(_width, _height, _format);

    switch (_type)
    {
        case eToneMappingFilterType::Uncharted2:
            m_shader = ShaderCollection::ToneMappingFilterUncharted2Shader();
            break;

        case eToneMappingFilterType::Reinhard:
            m_shader = ShaderCollection::ToneMappingFilterReinhardShader();
            break;

        case eToneMappingFilterType::WhitePreservingReinhard:
            m_shader = ShaderCollection::ToneMappingFilterWhitePreservingReinhardShader();
            break;

        case eToneMappingFilterType::LumaBasedReinhard:
            m_shader = ShaderCollection::ToneMappingFilterLumaBasedReinhardShader();
            break;

        case eToneMappingFilterType::RombDaHouse:
            m_shader = ShaderCollection::ToneMappingFilterRombDaHouseShader();
            break;

        case eToneMappingFilterType::Filmic:
            m_shader = ShaderCollection::ToneMappingFilterFilmicShader();
            break;

        case eToneMappingFilterType::Linear:
            m_shader = ShaderCollection::ToneMappingFilterLinearShader();
            break;

        default:
            JAM_ASSERT(false, "Unknown tone mapping filter type");
            break;
    }
}

void ToneMappingFilter::Bind(const Texture2D& _inputTexture)
{
    ImageFilter::Bind(_inputTexture);
    m_shader.Bind();
}

void SamplingFilter::Initialize(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format)
{
    InitializeFilterFrame_(_width, _height, _format);
}

void SamplingFilter::Bind(const Texture2D& _inputTexture)
{
    ImageFilter::Bind(_inputTexture);
    ShaderCollection::SamplingFilterShader().Bind();
}

}   // namespace jam