#pragma once
#include "../JamEngine/RenderGraph.h"

enum class eImageFilterOutput
{
    Inject,   // 출력할 프레임버퍼 주입
    Create,   // 설정에 따라 자동 생성
    Final,    // 최종 패스로 설정
};

enum class eImageFilter
{
    Blit,             // pass through. 입력값 없음
    Mix,              // linear interpolation으로 input0, input1 섞어서 출력. fparam의 x값으로 보간 계수 전달.
    Mult,             // input0, input1 곱해서 출력. 입력값 없음
    ToneMapping,      // HDR 톤 매핑. input0는 HDR 이미지, input1은 톤 매핑에 필요한 LUT 텍스처. fparam의 x값으로 노출 계수 전달. 자세한 것은 쉐이더 참조.
    Convolution1DX,   // 1D convolution 필터. input0는 입력 이미지. Kernel
    Convolution1DY,   // 1D convolution 필터. input0는 입력 이미지. fparam의 x값으로 텍스처 좌표에서의 샘플 간격 전달. iparam의 x값으로 커널 반경 전달. 커널 가중치는 CBImageFilterKernel의 weights 배열에 전달.
    FXAA,             // Fast Approximate Anti-Aliasing. 입력값 없음
};

struct CBImageFilterParam
{
    constexpr static Int32 kSlot = 0;

    Vec4  fparam;
    Vec4i iparam;
};

struct CBImageFilterKernel
{
    constexpr static Int32 kSlot = 1;

    float weights[16];
    Int32 length;
    float padding[3];
};

struct ImageFilterInfo
{
    // 필터 타입
    eImageFilter filter = eImageFilter::Blit;

    // 출력
    eImageFilterOutput output = eImageFilterOutput::Inject;

    // output이 Inject일 때 사용.
    String frameBuffer = {};

    // output이 Create일 때 사용.
    eTextureFormat format         = eTextureFormat::Unknown;
    Int32          width          = 0;
    Int32          height         = 0;
    bool           bUseAutoResize = false;   // true: 화면 크기에 맞춰 자동 리사이즈. false: width/height 고정 크기 사용.
    float          scale          = 1.f;

    // 입력
    String input0Texture            = {};      // 입력 텍스처 0. 필터 불문 반드시 존재해야 함.
    bool   bUseTexture2DArrayInput0 = false;   // true: input0이 Texture2DArray. 인덱싱은 iparam.x 이용.
    String input1Texture            = {};      // 입력 텍스처 1. 필터에 따라 없을 수 있음.
    bool   bUseTexture2DArrayInput1 = false;   // true: input1이 Texture2DArray. 인덱싱은 iparam.y 이용.

    // 상수 버퍼
    // 빈 문자열: 패스마다 in-place 생성. UpdateParams/UpdateKernel에 패스 이름을 전달해 업데이트.
    // 이름 지정: RenderGraph에서 조회하여 사용. 필터 간 상수 버퍼 공유 시 사용.
    String paramCBufferOrEmpty  = {};
    String kernelCBufferOrEmpty = {};

    // 실행자
    // nullptr: ScreenTri 렌더링. 필요 시 커스텀 가능. 단, 반드시 ScreenTri를 렌더링해야함.
    Delegate<void()> executorOrNull = nullptr;
};

class ImageFilterManager
{
    constexpr static Int32 kTInput0Slot      = 0;
    constexpr static Int32 kTInput1Slot      = 1;
    constexpr static Int32 kTArrayInput0Slot = 2;
    constexpr static Int32 kTArrayInput1Slot = 3;
    constexpr static Int32 kSLinearClampSlot = 0;
    constexpr static Int32 kSLinearWrapSlot  = 1;

    struct ImageFilterSpec
    {
        String programName = {};

        // filesystem
        String              path   = {};
        Vector<ShaderMacro> macros = {};

        // cbuffer
        bool bNeedParam  = false;
        bool bNeedKernel = false;
        bool bNeedInput1 = false;
    };

public:
    ImageFilterManager(const ImageFilterManager&)                = delete;
    ImageFilterManager& operator=(const ImageFilterManager&)     = delete;
    ImageFilterManager(ImageFilterManager&&) noexcept            = delete;
    ImageFilterManager& operator=(ImageFilterManager&&) noexcept = delete;

    [[nodiscard]] static ImageFilterManager& GetInstance();
    static void                              CreateInstance();
    static void                              DestroyInstance();

    RenderGraph::RenderPassBuilder AddImageFilter(RenderGraph& _graph, StringView _name, const ImageFilterInfo& _desc);

    void UpdateParams(const RenderGraph& _graph, StringView _passName, const CBImageFilterParam& _param) const;
    void UpdateKernel(const RenderGraph& _graph, StringView _passName, const CBImageFilterKernel& _kernel) const;

    void SubmitScreenTriangle() const;

private:
    ImageFilterManager();
    ~ImageFilterManager();

    [[nodiscard]] String MakeParamBufferName_(String _passName) const;
    [[nodiscard]] String MakeKernelBufferName_(String _passName) const;

    [[nodiscard]] ProgramHandle GetOrLoadProgram_(eImageFilter _filter, bool _bUseTexture2DArrayInput0, bool _bUseTexture2DArrayInput1);
    [[nodiscard]] Int32         MakeProgramsIndex_(bool _bUseTexture2DArrayInput0, bool _bUseTexture2DArrayInput1) const;

    Ref<Mesh>                                m_pScreenTriangle  = nullptr;
    EnumArray<eImageFilter, ProgramHandle>   m_programs[4]      = {};
    EnumArray<eImageFilter, ImageFilterSpec> m_imageFilterSpecs = {};
    Int32                                    m_width            = 0;
    Int32                                    m_height           = 0;

    inline static ImageFilterManager* s_pInstance = nullptr;
};

[[nodiscard]] inline ImageFilterManager& GetImageFilterManager()
{
    return ImageFilterManager::GetInstance();
}
