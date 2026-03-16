#pragma once
#include "GraphicsBase.h"
#include "ImageBase.h"
#include "ResourcePool.h"

namespace Jam
{

class ShaderCompiler;
class WindowResizedEvent;
class WindowMaximizedEvent;
class Event;

class Graphics
{

    enum class ePipelineDirtyFlags : UInt32
    {
        None = 0,

        Primitive    = 1 << 0,
        VertexBuffer = 1 << 1,
        IndexBuffer  = 1 << 2,

        VSConstantBuffer = 1 << 3,
        PSConstantBuffer = 1 << 4,
        GSConstantBuffer = 1 << 5,
        HSConstantBuffer = 1 << 6,
        DSConstantBuffer = 1 << 7,
        CSConstantBuffer = 1 << 8,

        VSShaderResourceView = 1 << 9,
        PSShaderResourceView = 1 << 10,
        GSShaderResourceView = 1 << 11,
        HSShaderResourceView = 1 << 12,
        DSShaderResourceView = 1 << 13,
        CSShaderResourceView = 1 << 14,

        VSSamplerState = 1 << 15,
        PSSamplerState = 1 << 16,
        GSSamplerState = 1 << 17,
        HSSamplerState = 1 << 18,
        DSSamplerState = 1 << 19,
        CSSamplerState = 1 << 20,

        RasterizerState   = 1 << 21,
        BlendState        = 1 << 22,
        DepthStencilState = 1 << 23,
        Viewport          = 1 << 24,
        FrameBuffer       = 1 << 25,
        ScissorRect       = 1 << 26,

        Program = 1 << 26,

        InputLayout = 1 << 27,

        ConstantBufferMask = VSConstantBuffer
                           | PSConstantBuffer
                           | GSConstantBuffer
                           | HSConstantBuffer
                           | DSConstantBuffer
                           | CSConstantBuffer,

        ShaderResourceViewMask = VSShaderResourceView
                               | PSShaderResourceView
                               | GSShaderResourceView
                               | HSShaderResourceView
                               | DSShaderResourceView
                               | CSShaderResourceView,

        SamplerStateMask = VSSamplerState
                         | PSSamplerState
                         | GSSamplerState
                         | HSSamplerState
                         | DSSamplerState
                         | CSSamplerState,

        All = 0xFFFFFFFF
    };

    enum class eBufferType
    {
        VertexBuffer = 0,
        IndexBuffer,
        ConstantBuffer
    };

    struct ConstantBufferBinds
    {
        ConstantBufferBinds()
        {
            cbhs.fill(k_nullHandle);
        }

        Array<ConstantBufferHandle, k_maxNumConstantBufferSlots> cbhs       = {};
        Int32                                                    dirtyStart = Limits<Int32>::k_max;
        Int32                                                    dirtyEnd   = 0;
    };

    struct TextureBinds
    {
        TextureBinds()
        {
            texhs.fill(k_nullHandle);
        }

        Array<TextureHandle, k_maxNumShaderResourceViewSlots> texhs      = {};
        Int32                                                 dirtyStart = Limits<Int32>::k_max;
        Int32                                                 dirtyEnd   = 0;
    };

    struct SamplerBinds
    {
        Array<SamplerState, k_maxNumSamplerSlots> samplers     = {};
        Array<RGBA, k_maxNumSamplerSlots>         borderColors = {};
        Int32                                     dirtyStart   = Limits<Int32>::k_max;
        Int32                                     dirtyEnd     = 0;
    };

    struct RasterizerStateD3D11
    {
        eCullMode cullMode         = eCullMode::None;
        bool      bWireframe : 1   = false;
        bool      bDepthClamp : 1  = false;
        bool      bMultiSample : 1 = false;
        bool      bLineAA : 1      = false;
        bool      bScissor : 1     = false;
        bool      bCCWFront : 1    = false;
    };

    struct BlendStateD3D11
    {
        Array<BlendState, k_maxNumRenderTargetViewSlots> blends                = {};
        bool                                             bIndependentBlend : 1 = false;
        bool                                             bAlphaToCoverage : 1  = false;
    };

    struct DepthStencilStateD3D11
    {
        bool         bDepthWrite : 1   = false;
        eCompareFunc depthTestFunc : 4 = eCompareFunc::Less;
        StencilState fStencil          = StencilState::k_disable;
        StencilState bStencil          = StencilState::k_disable;
    };

public:
    Graphics(const Graphics&)                = delete;
    Graphics& operator=(const Graphics&)     = delete;
    Graphics(Graphics&&) noexcept            = delete;
    Graphics& operator=(Graphics&&) noexcept = delete;

    /// @brief 그래픽스 코어의 전역 인스턴스를 반환합니다.
    [[nodiscard]] static Graphics& GetInstance();

    /// @brief 내부 렌더링 리소스들의 메모리 누수를 검사합니다.
    bool DetectLeak();

    /// @brief 이벤트를 처리합니다.
    /// @param _event 처리할 이벤트 객체
    void OnEvent(Event& _event);

    /// @brief 화면에 렌더링 결과를 출력합니다.
    /// @param _bVSync 수직 동기화 사용 여부
    void Frame(bool _bVSync);

    /// @brief 수정 불가능한(Immutable) 버텍스 버퍼를 생성합니다.
    /// @param _vertData 버텍스 데이터
    /// @param _vl 버텍스 레이아웃
    /// @return 버텍스 버퍼에 대한 참조
    /// @note 일반적으로 사용하는 버텍스 버퍼이며, 성능이 뛰어납니다. 단, 생성 후에는 데이터를 수정할 수 없습니다.
    [[nodiscard]] VertexBufferHandle CreateVertexBuffer(
        CMemoryBlock        _vertData,
        const VertexLayout& _vl);

    /// @brief 수정 가능한(Dynamic) 버텍스 버퍼를 생성합니다.
    /// @param _numVerts 버텍스 개수
    /// @param _vl 버텍스 레이아웃
    /// @return 다이나믹 버텍스 버퍼에 대한 참조
    /// @note 자주 변경되는 버텍스 데이터를 저장하는데 사용합니다. 성능은 immutable 버텍스 버퍼보다 떨어집니다.
    [[nodiscard]] VertexBufferHandle CreateDynamicVertexBuffer(
        Int32               _numVerts,
        const VertexLayout& _vl);

    /// @brief 수정 불가능한(Immutable) 인덱스 버퍼를 생성합니다.
    /// @param _idxData 인덱스 데이터
    /// @param _bUseU32 16비트 또는 32비트 인덱스 포맷
    /// @return 인덱스 버퍼에 대한 참조
    /// @note 일반적으로 사용하는 인덱스 버퍼이며, 성능이 뛰어납니다. 단, 생성 후에는 데이터를 수정할 수 없습니다.
    [[nodiscard]] IndexBufferHandle CreateIndexBuffer(
        CMemoryBlock _idxData,
        bool         _bUseU32 = false);

    /// @brief 수정 가능한(Dynamic) 인덱스 버퍼를 생성합니다.
    /// @param _numIndices 인덱스 개수
    /// @param _bUseU32 16비트 또는 32비트 인덱스 포맷
    /// @return 다이나믹 인덱스 버퍼에 대한 참조
    /// @note 자주 변경되는 인덱스 데이터를 저장하는데 사용합니다. 성능은 immutable 인덱스 버퍼보다 떨어집니다.
    [[nodiscard]] IndexBufferHandle CreateDynamicIndexBuffer(
        Int32 _numIndices,
        bool  _bUseU32 = false);

    /// @brief 상수 버퍼를 생성합니다.
    /// @param _byteWidth 버퍼 크기 (바이트 단위, 16의 배수여야 함)
    /// @return 상수 버퍼에 대한 참조
    /// @note 상수 버퍼는 다이나믹 버퍼로 생성되며, CPU에서 매 프레임마다 데이터를 업데이트할 수 있습니다.
    [[nodiscard]] ConstantBufferHandle CreateConstantBuffer(
        Int32 _byteWidth);

    /// @brief 2D 텍스처를 생성합니다.
    /// @param _width 텍스처 너비
    /// @param _height 텍스처 높이
    /// @param _format 텍스처 포맷
    /// @param _arraySize 텍스처 배열 크기
    /// @param _flags 텍스처 생성 플래그
    /// @param _initDataOrEmpty 초기화 데이터 (없으면 빈 배열 전달)
    /// @return 2D 텍스처에 대한 참조
    /// @note _initData가 제공되면 immutable 텍스처를 생성하고, 아니면 default 텍스처를 생성합니다.
    /// 텍스처를 프레임 버퍼로 사용할 경우 _viewFlags에 eViewFlags_RenderTarget 또는 eViewFlags_DepthStencil 플래그를 포함해야 합니다.
    [[nodiscard]] TextureHandle CreateTexture2D(
        Int32                      _width,
        Int32                      _height,
        eTextureFormat             _format,
        Int32                      _arraySize       = 1,
        Flags<eTextureCreateFlags> _flags           = eTextureCreateFlags::None,
        Span<const CMemoryBlock>   _initDataOrEmpty = {});

    /// @brief 큐브맵 텍스처를 생성합니다.
    /// @param _width 텍스처 너비
    /// @param _height 텍스처 높이
    /// @param _format 텍스처 포맷
    /// @param _cubeCount 큐브맵 개수
    /// @param _flags 텍스처 생성 플래그
    /// @param _initDataOrEmpty 초기화 데이터 (없으면 빈 배열 전달)
    /// @return 큐브맵 텍스처에 대한 참조
    /// @note 초기화 데이터가 제공되면 immutable 텍스처로 생성하고, 아니면 default 텍스처를 생성합니다.
    [[nodiscard]] TextureHandle CreateTextureCube(
        Int32                      _width,
        Int32                      _height,
        eTextureFormat             _format,
        Int32                      _cubeCount       = 1,
        Flags<eTextureCreateFlags> _flags           = eTextureCreateFlags::None,
        Span<const CMemoryBlock>   _initDataOrEmpty = {});

    /// @brief 3D 텍스처를 생성합니다.
    /// @param _width 텍스처 너비
    /// @param _height 텍스처 높이
    /// @param _depth 텍스처 깊이
    /// @param _format 텍스처 포맷
    /// @param _texFlags 텍스처 생성 플래그
    /// @param _initDataOrEmpty 초기화 데이터 (없으면 빈 배열 전달)
    /// @return 3D 텍스처에 대한 참조
    /// @note 초기화 데이터가 제공되면 immutable 텍스처로 생성하고, 아니면 default 텍스처를 생성합니다.
    [[nodiscard]] TextureHandle CreateTexture3D(
        Int32                      _width,
        Int32                      _height,
        Int32                      _depth,
        eTextureFormat             _format,
        Flags<eTextureCreateFlags> _texFlags        = eTextureCreateFlags::None,
        Span<const CMemoryBlock>   _initDataOrEmpty = {});

    /// @brief 프레임 버퍼를 생성합니다.
    /// @param _attachments 프레임 버퍼에 첨부할 텍스처 핸들 배열
    /// @param _bDestroyTexture true로 설정하면 프레임 버퍼가 파괴될 때 텍스처도 함께 파괴됩니다.
    /// 텍스처를 여러 용도로 사용한다면 반드시 false로 설정해야 합니다.
    /// @return 프레임 버퍼에 대한 참조
    /// @note 렌더 타겟 텍스처는 최대 k_renderTargetCountMax개까지 첨부할 수 있습니다.
    /// @note 뎁스 스텐실 텍스처는 최대 1개까지 첨부할 수 있습니다.
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        Span<const TextureHandle> _attachments,
        bool                      _bDestroyTexture = false);

    /// @brief 쉐이더를 생성합니다.
    /// @param _shaderByteCode 쉐이더 바이트코드
    /// @return 셰이더에 대한 참조
    [[nodiscard]] ShaderHandle CreateShader(
        CMemoryBlock _shaderByteCode);

    /// @brief 프로그램을 생성합니다.
    /// @param _vsh 버텍스 셰이더 핸들
    /// @param _psh 픽셀 셰이더 핸들
    /// @param _gsh 지오메트리 셰이더 핸들 (k_nullHandle 전달 시 해당 셰이더 스테이지는 생성되지 않음)
    /// @param _dsh 도메인 셰이더 핸들 (k_nullHandle 전달 시 해당 셰이더 스테이지는 생성되지 않음)
    /// @param _hsh 홀 셰이더 핸들 (k_nullHandle 전달 시 해당 셰이더 스테이지는 생성되지 않음)
    /// @param _bDestroyShader true로 설정하면 프로그램이 파괴될 때 셰이더도 함께 파괴됩니다.
    /// @return 프로그램에 대한 참조
    /// @note nullopt가 전달되면 해당 셰이더 스테이지는 생성되지 않습니다.
    [[nodiscard]] ProgramHandle CreateProgram(
        ShaderHandle _vsh,
        ShaderHandle _psh,
        ShaderHandle _gsh,
        ShaderHandle _dsh,
        ShaderHandle _hsh,
        bool         _bDestroyShader = false);

    /// @brief 프로그램을 생성합니다. (버텍스 셰이더 + 픽셀 셰이더 버전)
    /// @param _vsh 버텍스 셰이더 핸들
    /// @param _psh 픽셀 셰이더 핸들
    /// @param _bDestroyShader true로 설정하면 프로그램이 파괴될 때 셰이더도 함께 파괴됩니다.
    /// @return 프로그램에 대한 참조
    [[nodiscard]] ProgramHandle CreateProgram(
        ShaderHandle _vsh,
        ShaderHandle _psh,
        bool         _bDestroyShader = false);

    /// @brief 버텍스 버퍼를 파괴합니다.
    /// @param _vbh 버텍스 버퍼 핸들
    void DestroyVertexBuffer(
        VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼를 파괴합니다.
    /// @param _ibh 인덱스 버퍼 핸들
    void DestroyIndexBuffer(
        IndexBufferHandle _ibh);

    /// @brief 텍스처를 파괴합니다.
    /// @param _texh 텍스처 핸들
    void DestroyTexture(
        TextureHandle _texh);

    /// @brief 프레임 버퍼를 파괴합니다.
    /// @param _fbh 프레임 버퍼 핸들
    void DestroyFrameBuffer(
        FrameBufferHandle _fbh);

    /// @brief 프로그램을 파괴합니다.
    /// @param _ph 프로그램 핸들
    void DestroyProgram(
        ProgramHandle _ph);

    /// @brief 셰이더를 파괴합니다.
    /// @param _sh 셰이더 핸들
    void DestroyShader(
        ShaderHandle _sh);

    /// @brief 상수 버퍼를 파괴합니다.
    /// @param _cbh 상수 버퍼 핸들
    void DestroyConstantBuffer(
        ConstantBufferHandle _cbh);

    /// @brief 버텍스 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _vbh 버텍스 버퍼 리소스
    /// @param _name 버텍스 버퍼 이름
    /// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
    void SetVertexBufferName(
        VertexBufferHandle _vbh,
        StringView         _name);

    /// @brief 인덱스 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _ibh 인덱스 버퍼 리소스
    /// @param _name 인덱스 버퍼 이름
    /// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
    void SetIndexBufferName(
        IndexBufferHandle _ibh,
        StringView        _name);

    /// @brief 상수 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _cbh 상수 버퍼 리소스
    /// @param _name 상수 버퍼 이름
    /// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
    void SetConstantBufferName(
        ConstantBufferHandle _cbh,
        StringView           _name);

    /// @brief 텍스처에 디버깅용 이름을 설정합니다.
    /// @param _texh 텍스처 리소스
    /// @param _name 텍스처 이름
    /// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
    void SetTextureName(
        TextureHandle _texh,
        StringView    _name);

    /// @brief 프레임 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _fbh 프레임 버퍼 리소스
    /// @param _name 프레임 버퍼 이름
    /// @note GPU 디버거에서 리소스를 식별하는데 사용됩니다.
    void SetFrameBufferName(
        FrameBufferHandle _fbh,
        StringView        _name);

    /// @brief 프로그램에 디버깅용 이름을 설정합니다.
    /// @param _sh 프로그램 리소스
    /// @param _name 프로그램 이름
    void SetShaderName(
        ShaderHandle _sh,
        StringView   _name);

    /// @brief 텍스처를 블릿합니다.
    /// @param _blitDstTexh 대상 텍스처
    /// @param _blitSrcTexh 원본 텍스처
    void Blit(
        TextureHandle _blitDstTexh,
        TextureHandle _blitSrcTexh);

    /// @brief 텍스처를 블릿합니다.
    /// @param _blitDstTexh 대상 텍스처
    /// @param _blitDstMipIndex 대상 MIP 인덱스
    /// @param _blitDstArrayIndex 대상 배열 인덱스
    /// @param _blitDstX 대상 X 오프셋
    /// @param _blitDstY 대상 Y 오프셋
    /// @param _blitDstZ 대상 Z 오프셋
    /// @param _blitSrcTexh 원본 텍스처
    /// @param _blitSrcMipIndex 원본 MIP 인덱스
    /// @param _blitSrcArrayIndex 원본 배열 인덱스
    /// @param _blitSrcX 원본 X 오프셋
    /// @param _blitSrcY 원본 Y 오프셋
    /// @param _blitSrcZ 원본 Z 오프셋
    /// @param _blitSrcWidthOrZero 원본 텍스처 너비
    /// @param _blitSrcHeightOrZero 원본 텍스처 높이
    /// @param _blitSrcDepthOrZero 원본 텍스처 깊이
    void Blit(
        TextureHandle _blitDstTexh,
        Int32         _blitDstMipIndex,
        Int32         _blitDstArrayIndex,
        Int32         _blitDstX,
        Int32         _blitDstY,
        Int32         _blitDstZ,
        TextureHandle _blitSrcTexh,
        Int32         _blitSrcMipIndex,
        Int32         _blitSrcArrayIndex,
        Int32         _blitSrcX,
        Int32         _blitSrcY,
        Int32         _blitSrcZ,
        Int32         _blitSrcWidthOrZero,
        Int32         _blitSrcHeightOrZero,
        Int32         _blitSrcDepthOrZero);

    /// @brief 2D 텍스처를 업로드합니다.
    /// @param _texh 텍스처 핸들
    /// @param _dstMipIndex 대상 MIP 인덱스
    /// @param _dstArrayIndex 대상 배열 인덱스
    /// @param _dstX X 오프셋
    /// @param _dstY Y 오프셋
    /// @param _uploadData 업로드할 데이터
    /// @param _uploadWidth 데이터 너비
    /// @param _uploadHeight 데이터 높이
    /// @param _uploadDataRowPitch 데이터 행 간격
    void UploadTexture2D(
        TextureHandle _texh,
        Int32         _dstMipIndex,
        Int32         _dstArrayIndex,
        Int32         _dstX,
        Int32         _dstY,
        CMemoryBlock  _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDataRowPitch);

    /// @brief 2D 텍스처를 업로드합니다.
    /// @param _texh 텍스처 핸들
    /// @param _uploadData 업로드할 데이터
    /// @param _uploadWidth 데이터 너비
    /// @param _uploadHeight 데이터 높이
    /// @param _uploadDataRowPitch 데이터 행 간격
    void UploadTexture2D(
        TextureHandle _texh,
        CMemoryBlock  _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDataRowPitch);

    /// @brief 큐브 맵 텍스처를 업로드합니다.
    /// @param _texh 텍스처 핸들
    /// @param _dstMipIndex MIP 인덱스
    /// @param _dstCubeIndex 큐브 인덱스
    /// @param _dstCubeFace 큐브 면
    /// @param _dstX X 오프셋
    /// @param _dstY Y 오프셋
    /// @param _uploadData 업로드할 데이터
    /// @param _uploadWidth 데이터 너비
    /// @param _uploadHeight 데이터 높이
    /// @param _uploadDataRowPitch 데이터 행 간격
    void UploadTextureCube(
        TextureHandle _texh,
        Int32         _dstMipIndex,
        Int32         _dstCubeIndex,
        eCubeFace     _dstCubeFace,
        Int32         _dstX,
        Int32         _dstY,
        CMemoryBlock  _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDataRowPitch);

    /// @brief 큐브 맵 텍스처를 업로드합니다.
    /// @param _texh 텍스처 핸들
    /// @param _dstCubeFace 큐브 면
    /// @param _uploadData 업로드할 데이터
    /// @param _uploadWidth 데이터 너비
    /// @param _uploadHeight 데이터 높이
    /// @param _uploadDataRowPitch 데이터 행 간격
    void UploadTextureCube(
        TextureHandle _texh,
        eCubeFace     _dstCubeFace,
        CMemoryBlock  _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDataRowPitch);

    /// @brief 3D 텍스처를 업로드합니다.
    /// @param _texh 텍스처 핸들
    /// @param _dstMipIndex MIP 인덱스
    /// @param _dstArrayIndex 배열 인덱스
    /// @param _dstX X 오프셋
    /// @param _dstY Y 오프셋
    /// @param _dstZ Z 오프셋
    /// @param _uploadData 업로드할 데이터
    /// @param _uploadWidth 데이터 너비
    /// @param _uploadHeight 데이터 높이
    /// @param _uploadDepth 데이터 깊이
    /// @param _uploadDataRowPitch 데이터 행 간격
    /// @param _uploadDataDepthPitch 데이터 깊이 간격
    void UploadTexture3D(
        TextureHandle _texh,
        Int32         _dstMipIndex,
        Int32         _dstArrayIndex,
        Int32         _dstX,
        Int32         _dstY,
        Int32         _dstZ,
        CMemoryBlock  _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDepth,
        Int32         _uploadDataRowPitch,
        Int32         _uploadDataDepthPitch);

    /// @brief 3D 텍스처를 업로드합니다.
    /// @param _texh 텍스처 핸들
    /// @param _uploadData 업로드할 데이터
    /// @param _uploadWidth 데이터 너비
    /// @param _uploadHeight 데이터 높이
    /// @param _uploadDepth 데이터 깊이
    /// @param _uploadDataRowPitch 데이터 행 간격
    /// @param _uploadDataDepthPitch 데이터 깊이 간격
    void UploadTexture3D(
        TextureHandle _texh,
        CMemoryBlock  _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDepth,
        Int32         _uploadDataRowPitch,
        Int32         _uploadDataDepthPitch);

    /// @brief 2D 텍스처에서 데이터를 다운로드합니다.
    /// @param _texh 텍스처 핸들
    /// @param _mipIndex 다운로드할 MIP 레벨
    /// @param _arrayIndex 다운로드할 배열 인덱스
    /// @return 다운로드된 텍스처 데이터
    /// @note 어떤 종류의 텍스처에도 사용할 수 있습니다.
    [[nodiscard]] Vector<Byte> DownloadTexture2D(
        TextureHandle _texh,
        Int32         _mipIndex   = 0,
        Int32         _arrayIndex = 0);

    /// @brief 큐브맵 텍스처에서 데이터를 다운로드합니다.
    /// @param _texh 텍스처 핸들
    /// @param _mipIndex 다운로드할 MIP 레벨
    /// @param _cubeIndex 다운로드할 큐브 인덱스 (0~5)
    /// @param _cubeFace 다운로드할 큐브 페이스
    /// @return 다운로드된 텍스처 데이터
    /// @note 어떤 종류의 텍스처에도 사용할 수 있습니다.
    [[nodiscard]] Vector<Byte> DownloadTextureCube(
        TextureHandle _texh,
        Int32         _mipIndex  = 0,
        Int32         _cubeIndex = 0,
        eCubeFace     _cubeFace  = eCubeFace::Right);

    /// @brief 3D 텍스처에서 데이터를 다운로드합니다.
    /// @param _texh 텍스처 핸들
    /// @param _mipIndex 다운로드할 MIP 레벨
    /// @return 다운로드된 텍스처 데이터
    /// @note 어떤 종류의 텍스처에도 사용할 수 있습니다.
    [[nodiscard]] Vector<Byte> DownloadTexture3D(
        TextureHandle _texh,
        Int32         _mipIndex = 0);

    /// @brief 텍스처를 ImageContainer로 캡처합니다.
    /// @param _texh 텍스처 핸들
    /// @return 캡처된 스크래치 이미지
    [[nodiscard]] ImageContainer Capture(
        TextureHandle _texh);

    /// @brief 멀티샘플링된 텍스처를 멀티샘플링되지 않은 텍스처로 해상합니다.
    /// @param _srcTexh MSAA 텍스처 핸들
    /// @param _srcArrayIndex MSAA 텍스처 배열 인덱스
    /// @param _dstTexh 해상할 텍스처 핸들
    /// @param _dstArrayIndex 해상할 텍스처 배열 인덱스
    /// @note mipmap 레벨은 0으로 고정됩니다.
    void Resolve(
        TextureHandle _srcTexh,
        Int32         _srcArrayIndex,
        TextureHandle _dstTexh,
        Int32         _dstArrayIndex);

    /// @brief 텍스처의 해상도를 반환합니다.
    /// @param _texh 텍스처 핸들
    /// @param _mipIndex 조회할 MIP 레벨 인덱스
    /// @return 텍스처 해상도
    TextureResolution GetTextureResolution(
        TextureHandle _texh,
        Int32         _mipIndex = 0);

    /// @brief 렌더 타겟을 특정 색상으로 클리어합니다.
    /// @param _fbh 프레임 버퍼 핸들 (k_invalidResource 사용 시 백버퍼)
    /// @param _color 클리어 색상
    /// @param _renderTargetIndex 클리어할 렌더 타겟 인덱스
    void ClearColor(
        FrameBufferHandle _fbh,
        RGBA              _color,
        Int32             _renderTargetIndex = 0);

    /// @brief 깊이 버퍼와 스텐실 버퍼를 클리어합니다.
    /// @param _fbh 프레임 버퍼 핸들 (k_invalidResource 사용 시 에러)
    /// @param _bClearDepth 깊이 클리어 여부
    /// @param _bClearStencil 스텐실 클리어 여부
    /// @param _depth 클리어할 깊이 값 (0.0 ~ 1.0)
    /// @param _stencil 클리어할 스텐실 값
    void ClearDepthStencil(
        FrameBufferHandle _fbh,
        bool              _bClearDepth,
        bool              _bClearStencil,
        float             _depth   = 1.f,
        UInt8             _stencil = 0);

    /// @brief 프레임 버퍼의 렌더 타겟 텍스처를 반환합니다.
    /// @param _fbh 프레임 버퍼 핸들  (k_invalidResource를 전달하면 백버퍼의 렌더 타겟 텍스처를 반환합니다.)
    /// @param _renderTargetIndex 렌더 타겟 인덱스
    /// render target texture가 없으면 assertion 처리합니다.
    /// 해당 함수로 반환된 텍스처는 DestroyTexture_()로 파괴하지 마십시오.
    [[nodiscard]] const TextureHandle& GetRenderTargetTexture(
        FrameBufferHandle _fbh,
        Int32             _renderTargetIndex = 0);

    /// @brief 프레임 버퍼의 깊이-스텐실 텍스처를 반환합니다.
    /// @param _fbh 프레임 버퍼 핸들 (k_invalidResource를 전달하면 에러)
    /// @note k_invalidResource를 전달하면 기본 프레임 버퍼의 깊이-스텐실 텍스처를 반환합니다.
    /// depth stencil texture가 없으면 assertion 처리합니다.
    /// 해당 함수로 반환된 텍스처는 DestroyTexture_()로 파괴하지 마십시오.
    [[nodiscard]] const TextureHandle& GetDepthStencilTexture(
        FrameBufferHandle _fbh);

    [[nodiscard]] const VertexLayout& GetVertexLayout(
        VertexLayoutHandle _vlh);

    [[nodiscard]] VertexBufferInfo GetVertexBufferInfo(
        VertexBufferHandle _vbh);

    [[nodiscard]] IndexBufferInfo GetIndexBufferInfo(
        IndexBufferHandle _ibh);

    [[nodiscard]] ConstantBufferInfo GetConstantBufferInfo(
        ConstantBufferHandle _cbh);

    [[nodiscard]] TextureInfo GetTextureInfo(
        TextureHandle _texh);

    [[nodiscard]] FrameBufferInfo GetFrameBufferInfo(
        FrameBufferHandle _fbh);

    [[nodiscard]] ShaderInfo GetShaderInfo(
        ShaderHandle _sh);

    [[nodiscard]] ProgramInfo GetProgramInfo(
        ProgramHandle _ph);

    /// @brief 버텍스 버퍼에 데이터를 업로드합니다.
    /// @param _vbh 업로드할 버텍스 버퍼 핸들
    /// @param _writeData 업로드할 버텍스 데이터
    /// @param _offset 버텍스 버퍼 내의 오프셋 (바이트 단위)
    /// @param _bDiscard true: 기존 데이터를 초기화, false: 기존 데이터를 유지하면서 부분 업로드
    /// @note 전체 업로드(= _bDiscard = true)가 더 빠릅니다. 부분 업로드는 성능이 떨어질 수 있기에 주의해서 사용해야 합니다.
    ///       Dynamic VertexBuffer에만 데이터를 업로드할 수 있습니다.
    void UploadBuffer(
        VertexBufferHandle _vbh,
        CMemoryBlock       _writeData,
        Int32              _offset   = 0,
        bool               _bDiscard = true);

    /// @brief 인덱스 버퍼에 데이터를 업로드합니다.
    /// @param _ibh 업로드할 인덱스 버퍼 핸들
    /// @param _writeData 업로드할 인덱스 데이터
    /// @param _offset 버퍼 내의 오프셋 (바이트 단위)
    /// @param _bDiscard true: 기존 데이터를 초기화, false: 기존 데이터를 유지하면서 부분 업로드
    /// @note 전체 업로드(= _bDiscard = true)가 더 빠릅니다. 부분 업로드는 성능이 떨어질 수 있기에 주의해서 사용해야 합니다.
    ///       Dynamic VertexBuffer에만 데이터를 업로드할 수 있습니다.
    void UploadBuffer(
        IndexBufferHandle _ibh,
        CMemoryBlock      _writeData,
        Int32             _offset   = 0,
        bool              _bDiscard = true);

    /// @brief 상수 버퍼에 데이터를 업로드합니다.
    /// @param _cbh 업로드할 상수 버퍼 핸들
    /// @param _writeData 업로드할 상수 버퍼 데이터
    /// @note 상수 버퍼는 부분 업로드가 불가능하며, 항상 버퍼 전체를 덮어씁니다.
    void UploadBuffer(
        ConstantBufferHandle _cbh,
        CMemoryBlock         _writeData);

    /// @brief 버텍스 버퍼에 대한 업로드용 메모리 뷰를 반환합니다.
    /// @param _vbh 업로드할 버텍스 버퍼 핸들
    /// @note Dynamic VertexBuffer에만 데이터를 업로드할 수 있습니다.
    ///       연속적으로 조금씩 데이터를 업로드할 때 유용합니다.
    ///       Begin을 호출한 후에는 반드시 EndUploadBuffer를 호출하여 업로드를 완료해야 합니다.
    ///       기존 버퍼의 내용을 모두 초기화됩니다.
    /// @return 업로드용 메모리 뷰
    [[nodiscard]] MemoryBlock BeginUploadBuffer(
        VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼에 대한 업로드용 메모리 뷰를 반환합니다.
    /// @param _ibh 업로드할 인덱스 버퍼 핸들
    /// @note Dynamic VertexBuffer에만 데이터를 업로드할 수 있습니다.
    ///       연속적으로 조금씩 데이터를 업로드할 때 유용합니다.
    ///       Begin을 호출한 후에는 반드시 EndUploadBuffer를 호출하여 업로드를 완료해야 합니다.
    ///       기존 버퍼의 내용을 모두 초기화됩니다.
    /// @return 업로드용 메모리 뷰
    [[nodiscard]] MemoryBlock BeginUploadBuffer(
        IndexBufferHandle _ibh);

    /// @brief 버텍스 버퍼에 대한 업로드를 완료합니다.
    /// @param _vbh 업로드할 버텍스 버퍼 핸들
    void EndUploadBuffer(
        VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼에 대한 업로드를 완료합니다.
    /// @param _ibh 업로드할 인덱스 버퍼 핸들
    void EndUploadBuffer(
        IndexBufferHandle _ibh);

    /// @brief 버텍스 버퍼에서 데이터를 다운로드합니다.
    /// @param _vbh 다운로드할 버텍스 버퍼 핸들
    /// @return 다운로드된 버퍼 데이터
    /// @note 어떤 종류의 버퍼에도 사용할 수 있습니다.
    [[nodiscard]] Vector<Byte> DownloadBuffer(
        VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼에서 데이터를 다운로드합니다.
    /// @param _ibh 다운로드할 인덱스 버퍼 핸들
    /// @return 다운로드된 버퍼 데이터
    /// @note 어떤 종류의 버퍼에도 사용할 수 있습니다.
    [[nodiscard]] Vector<Byte> DownloadBuffer(
        IndexBufferHandle _ibh);

    /// @brief 상수 버퍼에서 데이터를 다운로드합니다.
    /// @param _cbh 다운로드할 상수 버퍼 핸들
    /// @return 다운로드된 버퍼 데이터
    /// @note 어떤 종류의 버퍼에도 사용할 수 있습니다.
    [[nodiscard]] Vector<Byte> DownloadBuffer(
        ConstantBufferHandle _cbh);

    /// @brief 버텍스 버퍼의 버텍스 레이아웃을 반환합니다.
    /// @param _vbh 버텍스 버퍼 핸들
    /// @return 버텍스 레이아웃
    [[nodiscard]] const VertexLayout& GetVertexLayout(
        VertexBufferHandle _vbh);

    /// @brief 버텍스 버퍼를 파이프라인에 바인딩합니다.
    /// @param _vbh 버텍스 버퍼 핸들 (k_invalidResource 사용 시 바인딩 해제)
    void SetVertexBuffer(
        VertexBufferHandle _vbh);

    /// @brief 버텍스 버퍼를 오프셋과 개수를 지정하여 파이프라인에 바인딩합니다.
    /// @param _vbh 버텍스 버퍼 핸들
    /// @param _numVerts 버텍스 개수
    /// @param _vertOffset 버퍼 내의 오프셋 (버텍스 단위)
    void SetVertexBuffer(
        VertexBufferHandle _vbh,
        Int32              _numVerts,
        Int32              _vertOffset);

    /// @brief 인덱스 버퍼를 파이프라인에 바인딩합니다.
    /// @param _handleOrInvalid 인덱스 버퍼 핸들 (k_invalidResource 사용 시 바인딩 해제)
    void SetIndexBuffer(
        IndexBufferHandle _handleOrInvalid);

    /// @brief 인덱스 버퍼를 오프셋과 개수를 지정하여 파이프라인에 바인딩합니다.
    /// @param _ibh 인덱스 버퍼 핸들
    /// @param _numIdxes 인덱스 개수
    /// @param _idxOffset 버퍼 내의 오프셋 (인덱스 단위)
    void SetIndexBuffer(
        IndexBufferHandle _ibh,
        Int32             _numIdxes,
        Int32             _idxOffset);

    /// @brief 상수 버퍼를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _cbh 바인딩할 상수 버퍼 핸들 (k_invalidResource 사용 시 바인딩 해제)
    /// @param _shader 바인딩할 셰이더 스테이지
    /// @param _slot 바인딩 슬롯
    void SetConstantBuffer(
        ConstantBufferHandle _cbh,
        eShader              _shader,
        Int32                _slot);

    /// @brief 텍스처를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _texh 바인딩할 텍스처 핸들 (k_invalidResource 사용 시 바인딩 해제)
    /// @param _shader 바인딩할 셰이더 스테이지
    /// @param _slot 바인딩 슬롯
    void SetTexture(
        TextureHandle _texh,
        eShader       _shader,
        Int32         _slot);

    /// @brief 샘플러 상태를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _state 샘플러 상태 플래그
    /// @param _shader 바인딩할 셰이더 스테이지
    /// @param _slot 바인딩 슬롯
    void SetSampler(
        SamplerState _state,
        eShader      _shader,
        Int32        _slot);

    /// @brief Border 샘프러 상태의 테두리 색상을 설정합니다.
    /// @param _shader 셰이더 스테이지
    /// @param _slot 텍스처 슬롯 번호 (0 ~ 15)
    /// @param _rgba 테두리 색상
    void SetBorderColor(
        eShader _shader,
        Int32   _slot,
        RGBA    _rgba);

    /// @brief 렌더링 상태를 설정합니다.
    /// @param _state 렌더링 상태 플래그
    void SetRenderState(
        RenderState _state);

    /// @brief 블렌드 상태를 설정합니다.
    /// @param _state 블렌드 상태 플래그
    /// @param _renderTargetIndex 렌더 타겟 인덱스 (0 ~ 7)
    void SetBlendState(
        BlendState _state,
        Int32      _renderTargetIndex);

    /// @brief 블렌드 팩터를 설정합니다.
    /// @param _rgba 블렌딩 시 사용할 색상 팩터
    void SetBlendFactor(
        RGBA _rgba);

    /// @brief 스텐실 테스트 설정을 구성합니다.
    /// @param _fStencil 전면 스텐실 상태
    /// @param _bStencil 후면 스텐실 상태
    /// @param _stencilRef 스텐실 참조 값
    void SetStencil(
        StencilState _fStencil,
        StencilState _bStencil,
        UInt8        _stencilRef = 0);

    /// @brief 프로그램을 파이프라인에 바인딩합니다.
    /// @param _ph 프로그램 핸들 (k_invalidResource 사용 시 바인딩 해제)
    void SetProgram(
        ProgramHandle _ph);

    /// @brief 프레임 버퍼를 렌더 타겟으로 설정합니다.
    /// @param _fbh 프레임 버퍼 핸들 (k_invalidResource 사용 시 기본 프레임 버퍼)
    void SetFrameBuffer(
        FrameBufferHandle _fbh);

    /// @brief 뷰포트를 설정합니다.
    /// @param _vp 뷰포트 설정
    void SetViewport(
        Viewport _vp);

    /// @brief 뷰포트를 개별 매개변수로 설정합니다.
    /// @param _x 뷰포트 X 좌표
    /// @param _y 뷰포트 Y 좌표
    /// @param _width 뷰포트 너비
    /// @param _height 뷰포트 높이
    void SetViewport(
        UInt16 _x,
        UInt16 _y,
        UInt16 _width,
        UInt16 _height);

    /// @brief 시저링 영역을 설정합니다.
    /// @param _rect 시저링 영역
    void SetScissorRect(
        Rect _rect);

    /// @brief 시저링 영역을 개별 매개변수로 설정합니다.
    /// @param _left 시저링 영역의 왼쪽 좌표
    /// @param _top 시저링 영역의 위쪽 좌표
    /// @param _right 시저링 영역의 오른쪽 좌표
    /// @param _bottom 시저링 영역의 아래쪽 좌표
    void SetScissorRect(
        UInt16 _left,
        UInt16 _top,
        UInt16 _right,
        UInt16 _bottom);

    /// @brief 드로우 콜 제출 시 추가 매개변수를 설정합니다.
    /// @param _type 설정할 매개변수 타입
    /// @param _value 매개변수 값
    void SetSubmitParameter(
        eSubmitParameterType _type,
        Int32                _value);

    /// @brief 현재 파이프라인 바인딩 상태를 플러시합니다.
    /// @note 드로우 콜을 발생시키지 않고 즉시 상태를 적용합니다.
    void Flush();

    /// @brief 프로그램에서 특정 셰이더 스테이지의 셰이더를 반환합니다.
    /// @param _ph 프로그램 핸들
    /// @param _stage 셰이더 스테이지
    /// @return 셰이더 핸들 (해당 스테이지에 셰이더가 없으면 assertion 처리)
    /// 해당 함수로 얻는 쉐이더는 DestroyShader로 파괴하지 마십시오.
    [[nodiscard]] ShaderHandle GetShader(
        ProgramHandle _ph,
        eShader       _stage);

    /// @brief 백 버퍼의 해상도를 반환합니다.
    /// @return 백 버퍼 해상도
    [[nodiscard]] Vec2i GetBackBufferResolution() const;

    /// @brief 프레임 버퍼를 생성합니다. (simple)
    /// @param _texh 프레임 버퍼에 첨부할 단일 텍스처 핸들
    /// @param _bDestroyTexture true로 설정하면 프레임 버퍼가 파괴될 때 텍스처도 함께 파괴됩니다.
    /// @return 프레임 버퍼에 대한 참조
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        TextureHandle _texh,
        bool          _bDestroyTexture = false);

    /// @brief 프레임 버퍼를 생성합니다. (렌더 타겟 + 뎁스 스텐실)
    /// @param _width 프레임 버퍼의 너비
    /// @param _height 프레임 버퍼의 높이
    /// @param _renderTargetFormat 렌더 타겟 텍스처 포맷
    /// @param _depthFormat 뎁스 스텐실 텍스처 포맷
    /// @param _writeOnlyRenderTarget 렌더 타겟을 쓰기 전용으로 생성할지 여부
    /// @param _writeOnlyDepthStencil 뎁스 스텐실을 쓰기 전용으로 생성할지 여부
    /// @return 프레임 버퍼에 대한 참조
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        Int32          _width,
        Int32          _height,
        eTextureFormat _renderTargetFormat,
        eTextureFormat _depthFormat,
        bool           _writeOnlyRenderTarget = false,
        bool           _writeOnlyDepthStencil = false);

    /// @brief 프레임 버퍼를 생성합니다. (렌더 타겟 혹은 뎁스 스텐실 중 하나)
    /// @param _width 프레임 버퍼의 너비
    /// @param _height 프레임 버퍼의 높이
    /// @param _format 렌더 타겟 텍스처 포맷
    /// @param _bWriteOnly 쓰기 전용으로 생성할지 여부
    /// @return 프레임 버퍼에 대한 참조
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        Int32          _width,
        Int32          _height,
        eTextureFormat _format,
        bool           _bWriteOnly = false);

    /// @brief ImageContainer로부터 텍스처를 생성합니다.
    [[nodiscard]] TextureHandle CreateTexture(
        const ImageContainer& _scratchImage);

    /// @brief 드로우 콜을 제출합니다.
    /// @note Submit을 호출하기 이전에 리소스를 바인딩해야 합니다.
    void Submit();

private:
    Graphics(HWND _hWnd, Int32 _width, Int32 _height);
    ~Graphics();

    static void CreateInstance(HWND _hwnd, Int32 _width, Int32 _height);
    static void DestroyInstance();

    void BindConstantBuffers_(eShader _stage);
    void BindShaderResourceViews_(eShader _shader);
    void BindSamplerStates_(eShader _shader);
    void BindPipeline_();

    // event handlers
    void OnResize_(Int32 _width, Int32 _height);
    void OnWindowResizedEvent_(const WindowResizedEvent& _event);
    void OnWindowMaxizeEvent_(const WindowMaximizedEvent& _event);

    // native resource factory
    [[nodiscard]] HardwareTextureRef CreateHardwarTexture_(Int32 _width, Int32 _height, Int32 _depth, eTextureFormat _format, Int32 _arraySize, Flags<eTextureCreateFlags> _flags, eTextureType _type, Span<const CMemoryBlock> _initDataOrEmpty) const;
    [[nodiscard]] HardwareBuffer     CreateHardwareBuffer_(Int32 _byteWidth, eBufferType _type, CMemoryBlock _initDataOrEmpty) const;
    [[nodiscard]] FrameBuffer        CreateFrameBuffer_(Span<const TextureHandle> _attachments, bool _bDestoryTexture);

    [[nodiscard]] ShaderHandle       GetOrCreateShader_(CMemoryBlock _byteCode);
    [[nodiscard]] VertexLayoutHandle GetOrCreateVertexLayout_(const VertexLayout& _vl);

    // buffer upload/download
    [[nodiscard]] MemoryBlock  BeginUploadBuffer_(const HardwareBuffer& _buffer) const;
    void                       EndUploadBuffer_(const HardwareBuffer& _buffer) const;
    void                       UploadBuffer_(const HardwareBuffer& _buffer, CMemoryBlock _uploadData, Int32 _offset, bool _bDiscard) const;
    [[nodiscard]] Vector<Byte> DownloadBuffer_(const HardwareBuffer& _buffer) const;
    void                       UploadTexture_(const HardwareTextureRef& _dstTexture, Int32 _dstMipIndex, Int32 _dstArrayIndex, Int32 _dstX, Int32 _dstY, Int32 _dstZ, CMemoryBlock _uploadData, Int32 _uploadWidth, Int32 _uploadHeight, Int32 _uploadDepth, Int32 _uploadDataRowPitch, Int32 _uploadDataDepthPitch) const;
    [[nodiscard]] Vector<Byte> DownloadTexture_(const HardwareTextureRef& _texture, Int32 _mipIndex, Int32 _arrayIndex) const;
    void                       Blit_(const HardwareTextureRef& _blitDstTexture, Int32 _blitDstMipIndex, Int32 _blitDstArrayIndex, Int32 _blitDstX, Int32 _blitDstY, Int32 _blitDstZ, const HardwareTextureRef& _blitSrcTexture, Int32 _blitSrcMipIndex, Int32 _blitSrcArrayIndex, Int32 _blitSrcX, Int32 _blitSrcY, Int32 _blitSrcZ, Int32 _blitSrcWidthOrZero, Int32 _blitSrcHeightOrZero, Int32 _blitSrcDepthOrZero) const;

    // d3d11 renderState cache
    [[nodiscard]] ID3D11RasterizerState*   GetOrCreateD3D11RasterizerState_(RasterizerStateD3D11 _state);
    [[nodiscard]] ID3D11BlendState*        GetOrCreateD3D11BlendState_(const BlendStateD3D11& _state);
    [[nodiscard]] ID3D11DepthStencilState* GetOrCreateD3D11DepthStencilState_(DepthStencilStateD3D11 _state);
    [[nodiscard]] ID3D11SamplerState*      GetOrCreateD3D11SamplerState_(SamplerState _state, RGBA _color);

    // misc
    [[nodiscard]] UInt32 QueryMaxMsaaQualityLevel_(DXGI_FORMAT _format, Int32 _numSamples) const;

    // d3d11 core object
    ID3D11Device*        m_pd3d11Device        = nullptr;
    ID3D11DeviceContext* m_pd3d11DeviceContext = nullptr;
    IDXGISwapChain1*     m_pdxgiSwapchain      = nullptr;

    // resource pools
    ResourcePool<VertexBuffer>       m_vertexBuffers = {};
    ResourcePool<IndexBuffer>        m_indexBuffers  = {};
    ResourcePool<HardwareTextureRef> m_textures      = {};
    ResourcePool<FrameBuffer>        m_frameBuffers  = {};
    ResourcePool<ShaderRef>          m_shaders       = {};
    ResourcePool<VertexLayoutRef>    m_vertexLayouts = {};
    ResourcePool<ConstantBuffer>     m_cbuffers      = {};
    ResourcePool<Program>            m_programs      = {};

    // resource caches
    CASHashMap<UInt64, ShaderHandle>       m_shaderCache = {};
    CASHashMap<UInt64, VertexLayoutHandle> m_vlCache     = {};

    // d3d11 renderState caches
    CASHashMap<UInt32, ID3D11RasterizerState*>   m_d3d11RasterizerStateCache   = {};
    CASHashMap<UInt64, ID3D11BlendState*>        m_d3d11BlendStateCache        = {};
    CASHashMap<UInt64, ID3D11DepthStencilState*> m_d3d11DepthStencilStateCache = {};
    CASHashMap<UInt64, ID3D11SamplerState*>      m_d3d11SamplerStateCache      = {};

    // pipeline bind renderState
    VertexLayoutHandle m_vlhBind      = k_nullHandle;
    VertexBufferHandle m_vbhBind      = k_nullHandle;
    Int32              m_vbOffsetBind = 0;
    Int32              m_numVertsBind = 0;

    IndexBufferHandle m_ibhBind        = k_nullHandle;
    Int32             m_ibOffsetBind   = 0;
    Int32             m_numIndicesBind = 0;

    FrameBufferHandle m_fbhBind = k_nullHandle;
    ProgramHandle     m_phBind  = k_nullHandle;

    EnumArray<eShader, ConstantBufferBinds> m_cbufferBinds = {};
    EnumArray<eShader, TextureBinds>        m_textureBinds = {};
    EnumArray<eShader, SamplerBinds>        m_samplerBinds = {};

    Viewport m_vpBind          = {};
    Rect     m_scissorRectBind = {};

    RenderState m_renderStateBind = {};
    RGBA        m_blendFactorBind = {};
    UInt8       m_stencilRefBind  = 0;

    ePrimitive             m_primBind              = static_cast<ePrimitive>(0xFF);   // invalid at start
    DepthStencilStateD3D11 m_depthStencilStateBind = {};                              // invalid at start
    RasterizerStateD3D11   m_rasterizerStateBind   = {};                              // invalid at start
    BlendStateD3D11        m_blendStateBind        = {};                              // invalid at start

    EnumArray<eSubmitParameterType, Int32> m_submitParams = {};
    Flags<ePipelineDirtyFlags>             m_dirtyFlags   = ePipelineDirtyFlags::All;

    // back buffer
    Vec2i             m_backBufferRes  = {};
    TextureHandle     m_backBufferTexh = k_nullHandle;
    FrameBufferHandle m_backBufferFbh  = k_nullHandle;

    inline static Graphics* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] inline Graphics& GetGraphics()
{
    return Graphics::GetInstance();
}

}   // namespace Jam

/// 핸들 파괴 매크로
#define JAM_DESTROY_GRAPHICS_HANDLE(_handle, _func) \
    JAM_BEGIN_MACRO_BLOCK                           \
    if (_handle)                                    \
    {                                               \
        (_func)((_handle));                         \
        (_handle) = Jam::k_nullHandle;              \
    }                                               \
    JAM_END_MACRO_BLOCK

#define JAM_DESTROY_VERTEX_BUFFER(_handle)   JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyVertexBuffer)
#define JAM_DESTROY_INDEX_BUFFER(_handle)    JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyIndexBuffer)
#define JAM_DESTROY_CONSTANT_BUFFER(_handle) JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyConstantBuffer)
#define JAM_DESTROY_PROGRAM(_handle)         JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyProgram)
#define JAM_DESTROY_TEXTURE(_handle)         JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyTexture)
#define JAM_DESTROY_FRAME_BUFFER(_handle)    JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyFrameBuffer)
#define JAM_DESTROY_SHADER(_handle)          JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyShader)
