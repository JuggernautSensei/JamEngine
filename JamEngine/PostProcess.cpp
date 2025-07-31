#include "pch.h"

#include "PostProcess.h"

#include "CBuffferCollection.h"
#include "StateCollection.h"
#include "ImageFilter.h"
#include "RenderStates.h"
#include "Renderer.h"
#include "ShaderBridge.h"

namespace jam
{

void PostProcess::Initialize(std::span<std::shared_ptr<ImageFilter>> _filters)
{
    m_filters = std::vector<std::shared_ptr<ImageFilter>>(_filters.begin(), _filters.end());
}

void PostProcess::Render(const Texture2D& _inputTexture) const
{
    // constants
    ConstantBufferCollection::Bind<CB_POSTPROCESS>(eShader::PixelShader, CB_POSTPROCESS_SLOT);

    // sampler
    StateCollection::SamplerLinearWrap().Bind(eShader::PixelShader, k_samplerLinearWrapSlot);
    StateCollection::SamplerPointClamp().Bind(eShader::PixelShader, k_samplerPointClampSlot);

    // shader + resource binding + rendering
    Texture2D inputTexture = _inputTexture;
    for (const std::shared_ptr<ImageFilter>& filter: m_filters)
    {
        // unbind previous render target
        Renderer::UnbindRenderTargetViews();

        // bind input texture (previous render target's texture)
        filter->Bind(inputTexture);

        // render
        Renderer::DrawFullScreenQuad();

        // bind output texture as input for the next filter
        inputTexture = filter->GetOutputTexture();
    }

    // unbind input shader resources (maybe depth texture, back buffer, etc.)
    Renderer::UnbindShaderResourceViews(eShader::PixelShader, k_postProcessInputTexture1Slot, 2);
}

PostProcessBuilder& PostProcessBuilder::AddSamplingFilter(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format)
{
    m_filterFlags |= eFilterFlags_Sampling;
    m_samplingWidth  = _width;
    m_samplingHeight = _height;
    m_samplingFormat = _format;

    return *this;
}

PostProcessBuilder& PostProcessBuilder::AddFXAAFilter(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const eFXAAQuality _quality)
{
    m_filterFlags |= eFilterFlags_FXAA;
    m_fxaaQuality = _quality;
    m_fxaaWidth   = _width;
    m_fxaaHeight  = _height;
    m_fxaaFormat  = _format;

    return *this;
}

PostProcessBuilder& PostProcessBuilder::AddToneMappingFilter(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const eToneMappingFilterType _type)
{
    m_filterFlags |= eFilterFlags_ToneMapping;
    m_toneMappingType   = _type;
    m_toneMappingWidth  = _width;
    m_toneMappingHeight = _height;
    m_toneMappingFormat = _format;

    return *this;
}

PostProcessBuilder& PostProcessBuilder::AddBloomFilter(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const UInt32 _bloomLevel, const Texture2D& _bloomDestinationTexture)
{
    JAM_ASSERT(_bloomLevel > 0, "Bloom level must be greater than 0");
    JAM_ASSERT(_width / _bloomLevel > 0 && _height / _bloomLevel > 0, "Bloom width and height must be greater than 0");

    m_filterFlags |= eFilterFlags_Bloom;
    m_bloomLevel  = _bloomLevel;
    m_bloomWidth  = _width;
    m_bloomHeight = _height;
    m_bloomFormat = _format;

    return *this;
}

PostProcessBuilder& PostProcessBuilder::AddFogFilter(const UInt32 _width, const UInt32 _height, const DXGI_FORMAT _format, const Texture2D& _depthTexture)
{
    m_filterFlags |= eFilterFlags_Fog;
    m_fogWidth     = _width;
    m_fogHeight    = _height;
    m_fogFormat    = _format;
    m_depthTexture = _depthTexture;

    return *this;
}

PostProcess PostProcessBuilder::Build(const Texture2D& _outputTexture) const
{
    std::vector<std::shared_ptr<ImageFilter>> filters;

    // 샘플링 필터
    if (m_filterFlags & eFilterFlags_Sampling)
    {
        std::shared_ptr<SamplingFilter> samplingFilter = std::make_shared<SamplingFilter>();
        samplingFilter->Initialize(m_samplingWidth, m_samplingHeight, m_samplingFormat);
        filters.push_back(samplingFilter);
    }

    // fog filter
    if (m_filterFlags & eFilterFlags_Fog)
    {
        std::shared_ptr<FogFilter> fogFilter = std::make_shared<FogFilter>();
        fogFilter->Initialize(m_fogWidth, m_fogHeight, m_fogFormat, m_depthTexture);
        filters.push_back(fogFilter);
    }

    // bloom filter
    if (m_filterFlags & eFilterFlags_Bloom)
    {
        // 블룸 필터는 combine을 위해서 destination 텍스처가 필요함
        // filter가 비어있을 경우 더미 필터를 하나 생성함
        if (filters.empty())
        {
            std::shared_ptr<SamplingFilter> samplingFilter = std::make_shared<SamplingFilter>();
            samplingFilter->Initialize(m_bloomWidth, m_bloomHeight, m_bloomFormat);
            filters.push_back(samplingFilter);
        }

        // bloom destination
        const std::shared_ptr<ImageFilter>& lastFilter              = filters.back();
        const Texture2D&                    bloomDestinationTexture = lastFilter->GetOutputTexture();

        // down filters
        for (UInt32 i = 0; i < m_bloomLevel; i++)
        {
            UInt32                          bloomWidth     = m_bloomWidth / (1 << i);
            UInt32                          bloomHeight    = m_bloomHeight / (1 << i);
            std::shared_ptr<BlurDownFilter> blurDownFilter = std::make_shared<BlurDownFilter>();
            blurDownFilter->Initialize(bloomWidth, bloomHeight, m_bloomFormat);
            filters.push_back(blurDownFilter);
        }

        // up filters
        for (UInt32 i = m_bloomLevel; i > 0; i--)
        {
            UInt32                        bloomWidth   = m_bloomWidth / (1 << (i - 1));
            UInt32                        bloomHeight  = m_bloomHeight / (1 << (i - 1));
            std::shared_ptr<BlurUpFilter> blurUpFilter = std::make_shared<BlurUpFilter>();
            blurUpFilter->Initialize(bloomWidth, bloomHeight, m_bloomFormat);
            filters.push_back(blurUpFilter);
        }

        // combine filter
        std::shared_ptr<CombineFilter> combineFilter = std::make_shared<CombineFilter>();
        combineFilter->Initialize(m_bloomWidth, m_bloomHeight, m_bloomFormat, bloomDestinationTexture);
        filters.push_back(combineFilter);
    }

    // Tone Mapping filter
    if (m_filterFlags & eFilterFlags_ToneMapping)
    {
        std::shared_ptr<ToneMappingFilter> toneMappingFilter = std::make_shared<ToneMappingFilter>();
        toneMappingFilter->Initialize(m_toneMappingWidth, m_toneMappingHeight, m_toneMappingFormat, m_toneMappingType);
        filters.push_back(toneMappingFilter);
    }

    // FXAA filter
    if (m_filterFlags & eFilterFlags_FXAA)
    {
        std::shared_ptr<FXAAFilter> fxaaFilter = std::make_shared<FXAAFilter>();
        fxaaFilter->Initialize(m_fxaaWidth, m_fxaaHeight, m_fxaaFormat, m_fxaaQuality);
        filters.push_back(fxaaFilter);
    }

    // Set the output texture for the last filter
    const std::shared_ptr<ImageFilter>& lastFilter = filters.back();
    lastFilter->SetOutputTexture(_outputTexture);

    // Create and return the PostProcess objectd
    PostProcess postProcess;
    postProcess.Initialize(filters);
    return postProcess;
}

}   // namespace jam