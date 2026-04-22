#pragma once
#include "GfxBase.h"

namespace jam
{

enum class eFilterType
{
    FXAA_VeryLow,
    FXAA_Low,
    FXAA_Medium,
    FXAA_High,
    FXAA_VeryHigh,
    FXAA_Ultra,

    ToneMapping_Uncharted2,
    ToneMapping_Reinhard,
    ToneMapping_LumaBasedReinhard,
    ToneMapping_WhitePreservingReinhard,
    ToneMapping_RomBinDaHouse,
    ToneMapping_FilmicTone,
    ToneMapping_LinearTone,

    Sampling,

    Bloom,
    Bloomx2,
    Bloomx4,
    Bloomx6,
    Bloomx8,

    Fog
};

class PostProcess
{
    struct Filter
    {
        eFilterType       type;
        Viewport          viewport;
        FrameBufferHandle frameBufferHandle;
        TextureHandle     textureHandleCache;
        ProgramHandle     programHandle;
    };

public:
    PostProcess();
    ~PostProcess() { Reset(); }

    PostProcess(const PostProcess&)                = delete;
    PostProcess& operator=(const PostProcess&)     = delete;
    PostProcess(PostProcess&&) noexcept;
    PostProcess& operator=(PostProcess&&) noexcept;

    /// @brief 포스트 프로세스 필터 추가
    /// @param _type 필터 타입
    /// @param _width 필터의 너비
    /// @param _height 필터의 높이
    /// @param _format 필터의 포맷
    void AddFilter(
        eFilterType _type,
        Int32       _width,
        Int32       _height,
        eFormat     _format);

    /// @brief 포스트 프로세스 실행
    /// @param _inputTexture 입력 텍스처
    /// @param _outputFrameBuffer 출력 프레임 버퍼
    void Submit(
        TextureHandle     _inputTexture,
        FrameBufferHandle _outputFrameBuffer);

    /// @brief 포스트 프로세스 리셋
    void Reset();

private:
    std::vector<Filter>  m_filters;
    ConstantBufferHandle m_postProcessCbufferHandle = kInvalidHandle;
};

}   // namespace jam