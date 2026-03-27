#pragma once
#include "GraphicsBase.h"
#include "ImageBase.h"
#include "ResourcePool.h"

// TODO 바인딩 후 리소스 파괴 테스트. -> brute force한 해결법. 해제될떄 바인드 상태 전부 검사

namespace Jam
{

class ShaderCompiler;
class WindowResizedEvent;
class WindowMaximizedEvent;
class Event;

/// @brief D3D11 기반 그래픽스 백엔드.
///
/// 버퍼, 텍스처, 셰이더 등 GPU 리소스의 생성/파괴를 담당하며,
/// Dirty Flag 기반의 파이프라인 상태 관리와 드로우 콜 제출 기능을 제공합니다.
class Graphics
{
    enum class ePipelineDirtyFlags : UInt32
    {
        None = 0,

        Primitive    = 1 << 0,
        VertexBuffer = 1 << 1,
        IndexBuffer  = 1 << 2,

        VSConstantBuffer = 1 << 4,
        PSConstantBuffer = 1 << 5,
        GSConstantBuffer = 1 << 6,
        HSConstantBuffer = 1 << 7,
        DSConstantBuffer = 1 << 8,
        CSConstantBuffer = 1 << 9,

        VSShaderResourceView = 1 << 10,
        PSShaderResourceView = 1 << 11,
        GSShaderResourceView = 1 << 12,
        HSShaderResourceView = 1 << 13,
        DSShaderResourceView = 1 << 14,
        CSShaderResourceView = 1 << 15,

        VSSamplerState = 1 << 16,
        PSSamplerState = 1 << 17,
        GSSamplerState = 1 << 18,
        HSSamplerState = 1 << 19,
        DSSamplerState = 1 << 20,
        CSSamplerState = 1 << 21,

        RasterizerState   = 1 << 22,
        BlendState        = 1 << 23,
        DepthStencilState = 1 << 24,
        Viewport          = 1 << 25,
        FrameBuffer       = 1 << 26,
        ScissorRect       = 1 << 27,

        Program        = 1 << 28,
        ComputeProgram = 1 << 29,

        InstanceBuffer = 1 << 30,
        InputLayout    = 1 << 31,
        End            = InputLayout,
        All            = (End << 1) - 1,

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
                         | CSSamplerState
    };

    enum class eBufferType
    {
        VertexBuffer = 0,
        IndexBuffer,
        ConstantBuffer,
    };

    struct ConstantBufferBinds
    {
        ConstantBufferBinds()
        {
            cbhs.fill(k_nullHandle);
        }

        Array<ConstantBufferHandle, k_maxNumConstantBufferSlots> cbhs       = {};
        Array<ID3D11Buffer*, k_maxNumConstantBufferSlots>        binder     = {};
        Int32                                                    dirtyStart = k_maxNumConstantBufferSlots;
        Int32                                                    dirtyEnd   = 0;
    };

    struct TextureBinds
    {
        TextureBinds()
        {
            texhs.fill(k_nullHandle);
        }

        Array<TextureHandle, k_maxNumShaderResourceViewSlots>             texhs      = {};
        Array<ID3D11ShaderResourceView*, k_maxNumShaderResourceViewSlots> binder     = {};
        Int32                                                             dirtyStart = k_maxNumShaderResourceViewSlots;
        Int32                                                             dirtyEnd   = 0;
    };

    struct SamplerBinds
    {
        struct Sampler
        {
            Flags<eSamplerFlags> state = {};
            RGBA                 rgba  = {};
        };

        Array<Sampler, k_maxNumSamplerSlots>             samplers   = {};
        Array<ID3D11SamplerState*, k_maxNumSamplerSlots> binder     = {};
        Int32                                            dirtyStart = k_maxNumSamplerSlots;
        Int32                                            dirtyEnd   = 0;
    };

    struct StreamBinds
    {
        struct Stream
        {
            VertexBufferHandle vbh    = k_nullHandle;
            VertexLayoutHandle vlh    = k_nullHandle;
            Int32              offset = 0;
        };

        Array<Stream, k_maxNumVertexStream> stream  = {};
        UInt32                              useMask = 0;
    };

    struct ReadbackTextureCacheEntry
    {
        ID3D11Resource* pResource = nullptr;
        Int32           width     = 0;
        Int32           height    = 0;
        Int32           depth     = 0;
    };

public:
    Graphics(const Graphics&)                = delete;
    Graphics& operator=(const Graphics&)     = delete;
    Graphics(Graphics&&) noexcept            = delete;
    Graphics& operator=(Graphics&&) noexcept = delete;

    ///////////////////////////////////////
    ///       INSTANCE MANAGEMENT       ///
    ///////////////////////////////////////

    /// @brief 그래픽스 코어의 전역 인스턴스를 반환합니다.
    /// @return Graphics 싱글턴 인스턴스
    /// @note CreateInstance 호출 이전에 사용하면 크래시가 발생합니다.
    [[nodiscard]] static Graphics& GetInstance();

    /// @brief 내부 렌더링 리소스들의 메모리 누수를 검사합니다.
    /// @return 누수가 감지되면 true, 아니면 false
    /// @note 감지된 리소스는 경고 로그로 출력됩니다.
    ///       소멸자에서 자동으로 호출됩니다.
    bool DetectLeak();

    ///////////////////////////////////////
    ///             SYSTEM              ///
    ///////////////////////////////////////

    /// @brief 시스템 이벤트를 처리합니다.
    /// @param _event 처리할 이벤트 객체
    /// @note 현재 WindowResizedEvent와 WindowMaximizedEvent를 처리합니다.
    void OnEvent(Event& _event);

    /// @brief 현재 프레임을 화면에 출력하고 다음 프레임을 준비합니다.
    /// @param _bVSync 수직 동기화 사용 여부
    /// @note 프레임 버퍼 바인딩이 초기화됩니다. 다음 드로우 콜 전에 SetFrameBuffer를 다시 호출해야 합니다.
    void Frame(bool _bVSync);

    ///////////////////////////////////////
    ///        RESOURCE CREATION        ///
    ///////////////////////////////////////

    ///////// BUFFER /////////

    /// @brief 수정 불가능한(Immutable) 버텍스 버퍼를 생성합니다.
    /// @param _vertData 초기 버텍스 데이터 (비어 있으면 안 됩니다)
    /// @param _vl 버텍스 레이아웃
    /// @return 생성된 버텍스 버퍼 핸들
    /// @note 생성 후에는 데이터를 수정할 수 없는 대신 GPU 접근 성능이 뛰어납니다.
    ///       정적인 메시 데이터에 사용하는 것을 권장합니다.
    [[nodiscard]] VertexBufferHandle CreateVertexBuffer(
        MemoryView          _vertData,
        const VertexLayout& _vl);

    /// @brief 수정 가능한(Dynamic) 버텍스 버퍼를 생성합니다.
    /// @param _numVerts 버텍스 최대 개수
    /// @param _vl 버텍스 레이아웃
    /// @return 생성된 다이나믹 버텍스 버퍼 핸들
    /// @note 매 프레임 CPU에서 데이터를 갱신해야 하는 경우에 사용합니다.
    ///       Immutable 버텍스 버퍼보다 GPU 접근 성능이 떨어지므로, 정적 데이터라면 CreateVertexBuffer를 사용하십시오.
    [[nodiscard]] VertexBufferHandle CreateDynamicVertexBuffer(
        Int32               _numVerts,
        const VertexLayout& _vl);

    /// @brief 인스턴스 버퍼를 생성합니다.
    /// @param _numInstances 인스턴스 최대 개수
    /// @param _instanceStride 각 인스턴스 데이터의 크기 (바이트 단위, 16의 배수여야 함)
    /// @return 생성된 인스턴스 버퍼 핸들
    [[nodiscard]] VertexBufferHandle CreateInstanceBuffer(
        Int32 _numInstances,
        Int32 _instanceStride);

    /// @brief 수정 불가능한(Immutable) 인덱스 버퍼를 생성합니다.
    /// @param _idxData 초기 인덱스 데이터 (비어 있으면 안 됩니다)
    /// @param _bUseU32 true: 32비트(UInt32) 인덱스 포맷, false: 16비트(UInt16) 인덱스 포맷
    /// @return 생성된 인덱스 버퍼 핸들
    /// @note 생성 후에는 데이터를 수정할 수 없는 대신 GPU 접근 성능이 뛰어납니다.
    ///       정적인 메시 인덱스에 사용하는 것을 권장합니다.
    [[nodiscard]] IndexBufferHandle CreateIndexBuffer(
        MemoryView _idxData,
        bool       _bUseU32 = false);

    /// @brief 수정 가능한(Dynamic) 인덱스 버퍼를 생성합니다.
    /// @param _numIndices 인덱스 최대 개수
    /// @param _bUseU32 true: 32비트(UInt32) 인덱스 포맷, false: 16비트(UInt16) 인덱스 포맷
    /// @return 생성된 다이나믹 인덱스 버퍼 핸들
    /// @note 매 프레임 CPU에서 데이터를 갱신해야 하는 경우에 사용합니다.
    ///       정적 데이터라면 CreateIndexBuffer를 사용하십시오.
    [[nodiscard]] IndexBufferHandle CreateDynamicIndexBuffer(
        Int32 _numIndices,
        bool  _bUseU32 = false);

    /// @brief 상수 버퍼를 생성합니다.
    /// @param _byteWidth 버퍼 크기 (바이트 단위, 16의 배수여야 하며 최대 65536바이트)
    /// @return 생성된 상수 버퍼 핸들
    /// @note 상수 버퍼는 내부적으로 Dynamic으로 생성되며, CPU에서 매 프레임 데이터를 업데이트할 수 있습니다.
    ///       크기는 반드시 16의 배수여야 합니다 (HLSL cbuffer 패킹 규칙).
    [[nodiscard]] ConstantBufferHandle CreateConstantBuffer(
        Int32 _byteWidth);

    [[nodiscard]] StorageBufferHandle CreateRawBuffer(
        Int32                            _byteWidth,
        Flags<eStorageBufferCreateFlags> _flags    = eStorageBufferCreateFlags::None,
        MemoryView                       _initData = {});

    [[nodiscard]] StorageBufferHandle CreateStructuredBuffer(
        Int32                            _numElements,
        Int32                            _stride,
        bool                             _bAppendConsume = false,
        bool                             _bCounter       = false,
        Flags<eStorageBufferCreateFlags> _flags          = eStorageBufferCreateFlags::None);

    [[nodiscard]] StorageBufferHandle CreateTypedBuffer(
        Int32                            _numElements,
        eStorageBufferDataType           _dataType,
        Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None);

    [[nodiscard]] StorageBufferHandle CreateReadbackBuffer(
        Int32 _byteWidth);

    /// @brief 버텍스 레이아웃을 생성합니다.
    /// @param _vl 버텍스 레이아웃 설명
    /// @return 생성된 버텍스 레이아웃 핸들
    /// @note 버텍스 버퍼를 GPU에 바인딩할 때 필요한 입력 레이아웃입니다. 동일한 레이아웃을 여러 버텍스 버퍼에서 공유할 수 있습니다.
    ///       버텍스 버퍼를 생성할 때, 자동으로 생성되기 때문에, 수동으로 생성할 필요는 없습니다.
    ///       그러나, 세밀한 조정이 필요한 경우, 수동으로 관리할 수 있습니다.
    [[nodiscard]] VertexLayoutHandle CreateVertexLayout(
        const VertexLayout& _vl);

    ////////// TEXTURE /////////

    /// @brief ImageContainer로부터 텍스처를 생성합니다.
    /// @param _imageContainer 원본 이미지 컨테이너 (2D, 3D, Cubemap 모두 지원)
    /// @param _bMutable true로 설정하면 생성 후에도 CPU에서 데이터를 수정할 수 있습니다. 기본적으로는 Immutable 텍스처로 생성됩니다.
    /// @return 생성된 텍스처 핸들
    /// @note 파일에서 읽어온 텍스처를 GPU에 올릴 때 사용합니다.
    ///       미리 생성된 MIP 체인이 있으면 그대로 업로드합니다.
    [[nodiscard]] TextureHandle CreateTexture(
        const ImageContainer& _imageContainer,
        bool                  _bMutable = false);

    /// @brief 2D 텍스처를 생성합니다.
    /// @param _width 텍스처 너비 (픽셀 단위, 1 이상)
    /// @param _height 텍스처 높이 (픽셀 단위, 1 이상)
    /// @param _format 텍스처 포맷
    /// @param _arraySize 텍스처 배열 크기 (기본: 1)
    /// @param _flags 텍스처 생성 플래그 (기본: None)
    /// @param _initDataOrEmpty 슬라이스별 초기화 데이터 배열. 비어 있으면 초기화되지 않은 텍스처가 생성됩니다.
    ///                         제공 시 배열 크기는 arraySize * numMips여야 합니다.
    /// @return 생성된 2D 텍스처 핸들
    /// @note _initDataOrEmpty가 제공되면 Immutable 텍스처를 생성합니다.
    ///       렌더 타겟으로 사용할 경우 _flags에 eTextureCreateFlags::RenderTarget을 포함해야 합니다.
    ///       깊이 스텐실로 사용할 경우 _flags에 eTextureCreateFlags::DepthStencil을 포함해야 합니다.
    [[nodiscard]] TextureHandle CreateTexture2D(
        Int32                      _width,
        Int32                      _height,
        eTextureFormat             _format,
        Int32                      _arraySize       = 1,
        Flags<eTextureCreateFlags> _flags           = eTextureCreateFlags::None,
        Span<const MemoryView>     _initDataOrEmpty = {});

    /// @brief 큐브맵 텍스처를 생성합니다.
    /// @param _width 각 큐브 면의 너비 (픽셀 단위, 1 이상)
    /// @param _height 각 큐브 면의 높이 (픽셀 단위, 1 이상, 일반적으로 _width와 동일)
    /// @param _format 텍스처 포맷
    /// @param _cubeCount 큐브맵 개수 (기본: 1). 내부 배열 크기는 _cubeCount * 6이 됩니다.
    /// @param _flags 텍스처 생성 플래그 (기본: None)
    /// @param _initDataOrEmpty 면별 초기화 데이터 배열. 비어 있으면 초기화되지 않은 텍스처가 생성됩니다.
    ///                         제공 시 배열 크기는 cubeCount * 6 * numMips여야 합니다.
    /// @return 생성된 큐브맵 텍스처 핸들
    [[nodiscard]] TextureHandle CreateTextureCube(
        Int32                      _width,
        Int32                      _height,
        eTextureFormat             _format,
        Int32                      _cubeCount       = 1,
        Flags<eTextureCreateFlags> _flags           = eTextureCreateFlags::None,
        Span<const MemoryView>     _initDataOrEmpty = {});

    /// @brief 3D 볼륨 텍스처를 생성합니다.
    /// @param _width 텍스처 너비 (픽셀 단위, 1 이상)
    /// @param _height 텍스처 높이 (픽셀 단위, 1 이상)
    /// @param _depth 텍스처 깊이 (슬라이스 수, 1 이상)
    /// @param _format 텍스처 포맷
    /// @param _texFlags 텍스처 생성 플래그 (기본: None)
    /// @param _initDataOrEmpty MIP 레벨별 초기화 데이터 배열. 비어 있으면 초기화되지 않은 텍스처가 생성됩니다.
    /// @return 생성된 3D 텍스처 핸들
    [[nodiscard]] TextureHandle CreateTexture3D(
        Int32                      _width,
        Int32                      _height,
        Int32                      _depth,
        eTextureFormat             _format,
        Flags<eTextureCreateFlags> _texFlags        = eTextureCreateFlags::None,
        Span<const MemoryView>     _initDataOrEmpty = {});

    ////////// FRAME BUFFER /////////

    /// @brief 프레임 버퍼를 생성합니다.
    /// @param _attachments 프레임 버퍼에 첨부할 텍스처 핸들 배열.
    ///                     깊이 포맷 텍스처는 자동으로 깊이-스텐실 어태치먼트로 인식됩니다.
    ///                     렌더 타겟은 최대 k_maxNumRenderTargetViewSlots개, 깊이-스텐실은 최대 1개까지 가능합니다.
    /// @param _bDestroyTexture true: 프레임 버퍼 파괴 시 첨부된 텍스처도 함께 파괴됩니다.
    ///                         false: 텍스처를 외부에서 독립적으로 관리합니다.
    ///                         텍스처를 다른 용도로도 사용한다면 반드시 false로 설정하십시오.
    /// @return 생성된 프레임 버퍼 핸들
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        Span<const TextureHandle> _attachments,
        bool                      _bDestroyTexture = false);

    /// @brief 단일 텍스처로 프레임 버퍼를 생성합니다.
    /// @param _texh 첨부할 텍스처 핸들 (렌더 타겟 또는 깊이-스텐실)
    /// @param _bDestroyTexture true: 프레임 버퍼 파괴 시 텍스처도 함께 파괴됩니다.
    /// @return 생성된 프레임 버퍼 핸들
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        TextureHandle _texh,
        bool          _bDestroyTexture = false);

    /// @brief 렌더 타겟과 깊이-스텐실 텍스처를 내부적으로 생성하여 프레임 버퍼를 만듭니다.
    /// @param _width 프레임 버퍼 너비 (픽셀 단위, 1 이상)
    /// @param _height 프레임 버퍼 높이 (픽셀 단위, 1 이상)
    /// @param _renderTargetFormat 렌더 타겟 텍스처 포맷 (깊이 포맷이면 안 됩니다)
    /// @param _depthFormat 깊이-스텐실 텍스처 포맷 (깊이 포맷이어야 합니다)
    /// @param _writeOnlyRenderTarget true: 렌더 타겟을 쓰기 전용으로 생성합니다 (SRV 없음, 샘플링 불가).
    /// @param _writeOnlyDepthStencil true: 깊이-스텐실을 쓰기 전용으로 생성합니다 (SRV 없음, 샘플링 불가).
    /// @return 생성된 프레임 버퍼 핸들
    /// @note 내부적으로 생성된 텍스처는 프레임 버퍼 파괴 시 자동으로 함께 파괴됩니다.
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        Int32          _width,
        Int32          _height,
        eTextureFormat _renderTargetFormat,
        eTextureFormat _depthFormat,
        bool           _writeOnlyRenderTarget = false,
        bool           _writeOnlyDepthStencil = false);

    /// @brief 렌더 타겟 또는 깊이-스텐실 텍스처 하나만으로 프레임 버퍼를 만듭니다.
    /// @param _width 프레임 버퍼 너비 (픽셀 단위, 1 이상)
    /// @param _height 프레임 버퍼 높이 (픽셀 단위, 1 이상)
    /// @param _format 텍스처 포맷. 깊이 포맷이면 깊이-스텐실로, 그 외는 렌더 타겟으로 자동 설정됩니다.
    /// @param _bWriteOnly true: 쓰기 전용으로 생성합니다 (SRV 없음, 샘플링 불가).
    /// @return 생성된 프레임 버퍼 핸들
    /// @note 내부적으로 생성된 텍스처는 프레임 버퍼 파괴 시 자동으로 함께 파괴됩니다.
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        Int32          _width,
        Int32          _height,
        eTextureFormat _format,
        bool           _bWriteOnly = false);

    ////////// / SHADER & PROGRAM /////////

    /// @brief 컴파일된 바이트코드로부터 셰이더를 생성합니다.
    /// @param _shaderByteCode 셰이더 DXBC 바이트코드
    /// @return 생성된 셰이더 핸들
    /// @note 동일한 바이트코드로 셰이더를 중복 생성하면 기존 셰이더의 참조 카운트를 증가시켜 반환합니다.
    ///       셰이더가 더 이상 필요없으면 DestroyShader로 파괴해야 합니다.
    ///       쉐이더 파일의 컴파일은 ShaderCompiler 클래스를 사용하여 수행할 수 있습니다.
    [[nodiscard]] ShaderHandle CreateShader(
        MemoryView _shaderByteCode);

    /// @brief 여러 셰이더 스테이지를 묶어 프로그램을 생성합니다.
    /// @param _vsh 버텍스 셰이더 핸들 (k_nullHandle 불가)
    /// @param _psh 픽셀 셰이더 핸들 (k_nullHandle 불가)
    /// @param _hsh 헐 셰이더 핸들. 테셀레이션을 사용하지 않으면 k_nullHandle을 전달하십시오.
    /// @param _dsh 도메인 셰이더 핸들. 테셀레이션을 사용하지 않으면 k_nullHandle을 전달하십시오.
    /// @param _gsh 지오메트리 셰이더 핸들. 지오메트리 셰이더를 사용하지 않으면 k_nullHandle을 전달하십시오.
    /// @param _bDestroyShader true: 프로그램 파괴 시 소유권을 가져가 셰이더도 함께 파괴합니다.
    ///                        false: 셰이더의 참조 카운트만 증가시킵니다. 셰이더를 별도로 파괴해야 합니다.
    /// @return 생성된 프로그램 핸들
    /// @note VS와 PS는 반드시 제공해야 합니다.
    ///       HS와 DS는 반드시 함께 제공하거나 둘 다 k_nullHandle이어야 합니다.
    [[nodiscard]] ProgramHandle CreateProgram(
        ShaderHandle _vsh,
        ShaderHandle _psh,
        ShaderHandle _hsh,
        ShaderHandle _dsh,
        ShaderHandle _gsh,
        bool         _bDestroyShader = false);

    /// @brief VS와 PS만으로 프로그램을 생성합니다.
    /// @param _vsh 버텍스 셰이더 핸들
    /// @param _psh 픽셀 셰이더 핸들
    /// @param _bDestroyShader true: 프로그램 파괴 시 셰이더도 함께 파괴합니다.
    /// @return 생성된 프로그램 핸들
    [[nodiscard]] ProgramHandle CreateProgram(
        ShaderHandle _vsh,
        ShaderHandle _psh,
        bool         _bDestroyShader = false);

    /// @brief Compute 셰이더로 프로그램을 생성합니다.
    /// @param _csh 컴퓨트 셰이더 핸들
    /// @param _bDestroyShader true: 프로그램 파괴 시 셰이더도 함께 파괴합니다.
    /// @return 생성된 프로그램 핸들
    [[nodiscard]] ProgramHandle CreateProgram(
        ShaderHandle _csh,
        bool         _bDestroyShader = false);

    ///////////////////////////////////////
    ///       RESOURCE DESTRUCTION      ///
    ///////////////////////////////////////

    /// @brief 버텍스 버퍼를 파괴합니다.
    /// @param _vbh 파괴할 버텍스 버퍼 핸들
    /// @note 편의 매크로 JAM_DESTROY_VERTEX_BUFFER 사용을 권장합니다.
    void DestroyVertexBuffer(VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼를 파괴합니다.
    /// @param _ibh 파괴할 인덱스 버퍼 핸들
    /// @note 편의 매크로 JAM_DESTROY_INDEX_BUFFER 사용을 권장합니다.
    void DestroyIndexBuffer(IndexBufferHandle _ibh);

    /// @brief 버텍스 레이아웃을 파괴합니다.
    /// @param _vlh 파괴할 버텍스 레이아웃 핸들
    /// @note  수동으로 관리할 때만 파괴할 필요가 있습니다.
    ///        편의 매크로 JAM_DESTROY_VERTEX_LAYOUT 사용을 권장합니다.
    void DestroyVertexLayout(VertexLayoutHandle _vlh);

    /// @brief 상수 버퍼를 파괴합니다.
    /// @param _cbh 파괴할 상수 버퍼 핸들
    /// @note 편의 매크로 JAM_DESTROY_CONSTANT_BUFFER 사용을 권장합니다.
    void DestroyConstantBuffer(ConstantBufferHandle _cbh);

    void DestroyStorageBuffer(StorageBufferHandle _sbh);

    /// @brief 텍스처를 파괴합니다.
    /// @param _texh 파괴할 텍스처 핸들
    /// @note 내부 참조 카운트가 0이 될 때 실제 D3D11 리소스가 해제됩니다.
    ///       FrameBuffer에 의해 관리되는 텍스처는 직접 파괴하지 마십시오.
    ///       편의 매크로 JAM_DESTROY_TEXTURE 사용을 권장합니다.
    void DestroyTexture(TextureHandle _texh);

    /// @brief 프레임 버퍼를 파괴합니다.
    /// @param _fbh 파괴할 프레임 버퍼 핸들
    /// @note _bDestroyTexture = true로 생성된 경우 첨부된 텍스처도 함께 파괴됩니다.
    ///       편의 매크로 JAM_DESTROY_FRAME_BUFFER 사용을 권장합니다.
    void DestroyFrameBuffer(FrameBufferHandle _fbh);

    /// @brief 셰이더를 파괴합니다.
    /// @param _sh 파괴할 셰이더 핸들
    /// @note 내부 참조 카운트가 0이 될 때 실제 D3D11 셰이더가 해제됩니다.
    ///       GetShader로 얻은 핸들은 파괴하지 마십시오. Program이 소유권을 가집니다.
    ///       편의 매크로 JAM_DESTROY_SHADER 사용을 권장합니다.
    void DestroyShader(ShaderHandle _sh);

    /// @brief 프로그램을 파괴합니다.
    /// @param _ph 파괴할 프로그램 핸들
    /// @note 내부적으로 각 셰이더의 참조 카운트를 감소시킵니다.
    ///       편의 매크로 JAM_DESTROY_PROGRAM 사용을 권장합니다.
    void DestroyProgram(ProgramHandle _ph);

    ///////////////////////////////////////
    ///          DEBUG NAMING           ///
    ///////////////////////////////////////

    /// @brief 버텍스 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _vbh 이름을 설정할 버텍스 버퍼 핸들
    /// @param _name 설정할 이름 (PIX, RenderDoc 등 GPU 디버거에서 표시됩니다)
    void SetVertexBufferName(VertexBufferHandle _vbh, StringView _name);

    /// @brief 인덱스 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _ibh 이름을 설정할 인덱스 버퍼 핸들
    /// @param _name 설정할 이름
    void SetIndexBufferName(IndexBufferHandle _ibh, StringView _name);

    /// @brief 상수 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _cbh 이름을 설정할 상수 버퍼 핸들
    /// @param _name 설정할 이름
    void SetConstantBufferName(ConstantBufferHandle _cbh, StringView _name);

    void SetStorageBufferName(StorageBufferHandle _sbh, StringView _name);

    /// @brief 텍스처에 디버깅용 이름을 설정합니다.
    /// @param _texh 이름을 설정할 텍스처 핸들
    /// @param _name 설정할 이름
    /// @note 텍스처 리소스와 SRV 모두에 이름이 설정됩니다.
    void SetTextureName(TextureHandle _texh, StringView _name);

    /// @brief 프레임 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _fbh 이름을 설정할 프레임 버퍼 핸들
    /// @param _name 설정할 이름
    /// @note RTV와 DSV 모두에 이름이 설정됩니다.
    void SetFrameBufferName(FrameBufferHandle _fbh, StringView _name);

    /// @brief 셰이더에 디버깅용 이름을 설정합니다.
    /// @param _sh 이름을 설정할 셰이더 핸들
    /// @param _name 설정할 이름
    void SetShaderName(ShaderHandle _sh, StringView _name);

    ///////////////////////////////////////
    ///       BUFFER DATA TRANSFER      ///
    ///////////////////////////////////////

    /// @brief 다이나믹 버텍스 버퍼에 데이터를 업로드합니다.
    /// @param _vbh 대상 버텍스 버퍼 핸들 (Dynamic 버퍼여야 합니다)
    /// @param _writeData 업로드할 데이터
    /// @param _offset 버퍼 내 오프셋 (바이트 단위, 기본: 0)
    /// @param _bDiscard true: 버퍼 전체를 무효화한 뒤 새 데이터를 씁니다 (더 빠름).
    ///                  false: 기존 데이터를 유지하면서 지정 범위만 부분 업데이트합니다 (더 느림).
    /// @note 부분 업데이트는 성능이 떨어질 수 있으므로 가능하면 _bDiscard = true를 사용하십시오.
    void UploadBuffer(
        VertexBufferHandle _vbh,
        MemoryView         _writeData,
        Int32              _offset   = 0,
        bool               _bDiscard = true);

    /// @brief 다이나믹 인덱스 버퍼에 데이터를 업로드합니다.
    /// @param _ibh 대상 인덱스 버퍼 핸들 (Dynamic 버퍼여야 합니다)
    /// @param _writeData 업로드할 데이터
    /// @param _offset 버퍼 내 오프셋 (바이트 단위, 기본: 0)
    /// @param _bDiscard true: 버퍼 전체를 무효화한 뒤 새 데이터를 씁니다 (더 빠름).
    ///                  false: 기존 데이터를 유지하면서 지정 범위만 부분 업데이트합니다 (더 느림).
    void UploadBuffer(
        IndexBufferHandle _ibh,
        MemoryView        _writeData,
        Int32             _offset   = 0,
        bool              _bDiscard = true);

    /// @brief 상수 버퍼에 데이터를 업로드합니다.
    /// @param _cbh 대상 상수 버퍼 핸들
    /// @param _writeData 업로드할 데이터. 크기는 버퍼의 byteWidth와 일치해야 합니다.
    /// @note 상수 버퍼는 항상 버퍼 전체를 덮어씁니다 (부분 업로드 불가).
    void UploadBuffer(
        ConstantBufferHandle _cbh,
        MemoryView           _writeData);

    /// @brief 버텍스 버퍼에 대한 매핑된 쓰기 포인터를 반환합니다.
    /// @param _vbh 매핑할 버텍스 버퍼 핸들 (Dynamic 버퍼여야 합니다)
    /// @return 버퍼 전체를 가리키는 쓰기 가능한 메모리 블록
    /// @note 이 함수를 호출하면 기존 버퍼 내용이 모두 초기화됩니다.
    ///       데이터를 다 쓴 후에는 반드시 EndUploadBuffer를 호출해야 합니다.
    ///       Begin ~ End 사이에 같은 버퍼에 UploadBuffer를 호출하면 안 됩니다.
    ///       해당 함수는 호출자에게 많이 책임을 넘기기 때문에, 가능하면 UploadBuffer 함수를 사용하는 것을 권장합니다.
    [[nodiscard]] MutableMemoryView BeginUploadBuffer(VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼에 대한 매핑된 쓰기 포인터를 반환합니다.
    /// @param _ibh 매핑할 인덱스 버퍼 핸들 (Dynamic 버퍼여야 합니다)
    /// @return 버퍼 전체를 가리키는 쓰기 가능한 메모리 블록
    /// @note 이 함수를 호출하면 기존 버퍼 내용이 모두 초기화됩니다.
    ///       데이터를 다 쓴 후에는 반드시 EndUploadBuffer를 호출해야 합니다.
    ///       해당 함수는 호출자에게 많이 책임을 넘기기 때문에, 가능하면 UploadBuffer 함수를 사용하는 것을 권장합니다.
    [[nodiscard]] MutableMemoryView BeginUploadBuffer(IndexBufferHandle _ibh);

    /// @brief 버텍스 버퍼의 매핑을 해제하여 업로드를 완료합니다.
    /// @param _vbh BeginUploadBuffer에 전달한 것과 동일한 핸들
    /// @note BeginUploadBuffer 호출 후 반드시 이 함수를 호출해야 합니다.
    void EndUploadBuffer(VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼의 매핑을 해제하여 업로드를 완료합니다.
    /// @param _ibh BeginUploadBuffer에 전달한 것과 동일한 핸들
    void EndUploadBuffer(IndexBufferHandle _ibh);

    ///////////////////////////////////////
    ///      TEXTURE DATA TRANSFER      ///
    ///////////////////////////////////////

    /// @brief 2D 텍스처의 특정 MIP/배열 슬라이스에 CPU 데이터를 업로드합니다.
    /// @param _texh 대상 텍스처 핸들 (Mutable 텍스처여야 합니다)
    /// @param _dstMipIndex 대상 MIP 레벨 인덱스 (0이 가장 큰 해상도)
    /// @param _dstArrayIndex 대상 배열 슬라이스 인덱스
    /// @param _dstX 대상 영역의 X 오프셋 (픽셀 단위)
    /// @param _dstY 대상 영역의 Y 오프셋 (픽셀 단위)
    /// @param _uploadData 업로드할 픽셀 데이터
    /// @param _uploadWidth 업로드 영역의 너비 (픽셀 단위)
    /// @param _uploadHeight 업로드 영역의 높이 (픽셀 단위)
    /// @param _uploadDataRowPitch 소스 데이터의 행 간격 (바이트 단위)
    void UploadTexture2D(
        TextureHandle _texh,
        Int32         _dstMipIndex,
        Int32         _dstArrayIndex,
        Int32         _dstX,
        Int32         _dstY,
        MemoryView    _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDataRowPitch);

    /// @brief 2D 텍스처의 MIP 0, 배열 슬라이스 0에 CPU 데이터를 업로드합니다.
    /// @param _texh 대상 텍스처 핸들 (Mutable 텍스처여야 합니다)
    /// @param _uploadData 업로드할 픽셀 데이터
    /// @param _uploadWidth 업로드 영역의 너비 (픽셀 단위)
    /// @param _uploadHeight 업로드 영역의 높이 (픽셀 단위)
    /// @param _uploadDataRowPitch 소스 데이터의 행 간격 (바이트 단위)
    void UploadTexture2D(
        TextureHandle _texh,
        MemoryView    _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDataRowPitch);

    /// @brief 큐브맵 텍스처의 특정 면에 CPU 데이터를 업로드합니다.
    /// @param _texh 대상 큐브맵 텍스처 핸들 (Mutable 텍스처여야 합니다)
    /// @param _dstMipIndex 대상 MIP 레벨 인덱스
    /// @param _dstCubeIndex 대상 큐브맵 인덱스 (큐브맵 배열 사용 시)
    /// @param _dstCubeFace 대상 큐브 면 (Right/Left/Top/Bottom/Front/Back)
    /// @param _dstX 대상 면 내의 X 오프셋 (픽셀 단위)
    /// @param _dstY 대상 면 내의 Y 오프셋 (픽셀 단위)
    /// @param _uploadData 업로드할 픽셀 데이터
    /// @param _uploadWidth 업로드 영역의 너비 (픽셀 단위)
    /// @param _uploadHeight 업로드 영역의 높이 (픽셀 단위)
    /// @param _uploadDataRowPitch 소스 데이터의 행 간격 (바이트 단위)
    void UploadTextureCube(
        TextureHandle _texh,
        Int32         _dstMipIndex,
        Int32         _dstCubeIndex,
        eCubeFace     _dstCubeFace,
        Int32         _dstX,
        Int32         _dstY,
        MemoryView    _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDataRowPitch);

    /// @brief 큐브맵의 MIP 0, 큐브 인덱스 0의 지정된 면에 CPU 데이터를 업로드합니다.
    /// @param _texh 대상 큐브맵 텍스처 핸들 (Mutable 텍스처여야 합니다)
    /// @param _dstCubeFace 대상 큐브 면
    /// @param _uploadData 업로드할 픽셀 데이터
    /// @param _uploadWidth 업로드 영역의 너비 (픽셀 단위)
    /// @param _uploadHeight 업로드 영역의 높이 (픽셀 단위)
    /// @param _uploadDataRowPitch 소스 데이터의 행 간격 (바이트 단위)
    void UploadTextureCube(
        TextureHandle _texh,
        eCubeFace     _dstCubeFace,
        MemoryView    _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDataRowPitch);

    /// @brief 3D 텍스처의 특정 영역에 CPU 데이터를 업로드합니다.
    /// @param _texh 대상 3D 텍스처 핸들 (Mutable 텍스처여야 합니다)
    /// @param _dstMipIndex 대상 MIP 레벨 인덱스
    /// @param _dstArrayIndex 대상 배열 인덱스 (3D 텍스처는 항상 0)
    /// @param _dstX, _dstY, _dstZ 대상 영역의 오프셋 (픽셀/슬라이스 단위)
    /// @param _uploadData 업로드할 볼륨 데이터
    /// @param _uploadWidth, _uploadHeight, _uploadDepth 업로드 영역의 크기
    /// @param _uploadDataRowPitch 소스 데이터의 행 간격 (바이트 단위)
    /// @param _uploadDataSlicePitch 소스 데이터의 슬라이스 간격 (바이트 단위)
    void UploadTexture3D(
        TextureHandle _texh,
        Int32         _dstMipIndex,
        Int32         _dstArrayIndex,
        Int32         _dstX,
        Int32         _dstY,
        Int32         _dstZ,
        MemoryView    _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDepth,
        Int32         _uploadDataRowPitch,
        Int32         _uploadDataSlicePitch);

    /// @brief 3D 텍스처의 MIP 0, 슬라이스 0 원점에 CPU 데이터를 업로드합니다.
    /// @param _texh 대상 3D 텍스처 핸들 (Mutable 텍스처여야 합니다)
    /// @param _uploadData 업로드할 볼륨 데이터
    /// @param _uploadWidth, _uploadHeight, _uploadDepth 업로드 데이터의 크기
    /// @param _uploadDataRowPitch 소스 데이터의 행 간격 (바이트 단위)
    /// @param _uploadDataSlicePitch 소스 데이터의 슬라이스 간격 (바이트 단위)
    void UploadTexture3D(
        TextureHandle _texh,
        MemoryView    _uploadData,
        Int32         _uploadWidth,
        Int32         _uploadHeight,
        Int32         _uploadDepth,
        Int32         _uploadDataRowPitch,
        Int32         _uploadDataSlicePitch);

    /// @brief 텍스처 데이터를 다른 텍스처로 GPU 복사합니다. (간략 버전)
    /// @param _dstTexh 대상 텍스처 (MIP 0, 슬라이스 0, 원점에 복사됩니다)
    /// @param _srcTexh 원본 텍스처 (MIP 0, 슬라이스 0, 전체 크기가 복사됩니다)
    /// @note 두 텍스처의 포맷이 호환되어야 합니다.
    void CopyTexture(
        TextureHandle _dstTexh,
        TextureHandle _srcTexh);

    /// @brief 텍스처의 지정된 영역을 다른 텍스처의 지정된 위치로 GPU 복사합니다.
    /// @param _dstTexh 대상 텍스처 핸들
    /// @param _dstMipIndex 대상 MIP 레벨
    /// @param _dstArrayIndex 대상 배열/슬라이스 인덱스
    /// @param _dstX, _dstY, _dstZ 대상 오프셋 (픽셀 단위)
    /// @param _srcTexh 원본 텍스처 핸들
    /// @param _srcMipIndex 원본 MIP 레벨
    /// @param _srcArrayIndex 원본 배열/슬라이스 인덱스
    /// @param _srcX, _srcY, _srcZ 원본 영역의 시작 오프셋 (픽셀 단위)
    /// @param _srcWidthOrZero 복사할 너비. 0이면 원본 텍스처 전체 너비를 사용합니다.
    /// @param _srcHeightOrZero 복사할 높이. 0이면 원본 텍스처 전체 높이를 사용합니다.
    /// @param _srcDepthOrZero 복사할 깊이. 0이면 원본 텍스처 전체 깊이를 사용합니다.
    void CopyTexture(
        TextureHandle _dstTexh,
        Int32         _dstMipIndex,
        Int32         _dstArrayIndex,
        Int32         _dstX,
        Int32         _dstY,
        Int32         _dstZ,
        TextureHandle _srcTexh,
        Int32         _srcMipIndex,
        Int32         _srcArrayIndex,
        Int32         _srcX,
        Int32         _srcY,
        Int32         _srcZ,
        Int32         _srcWidthOrZero,
        Int32         _srcHeightOrZero,
        Int32         _srcDepthOrZero);

    /// @brief 텍스처 전체를 DirectX::ScratchImage로 캡처합니다.
    /// @param _texh 캡처할 텍스처 핸들
    /// @return 캡처된 이미지 컨테이너 (파일 저장 등에 활용 가능)
    /// @note GPU-CPU 동기화가 발생합니다. 성능이 중요한 경로에서는 사용하지 마십시오.
    [[nodiscard]] ImageContainer Capture(TextureHandle _texh);

    ///////////////////////////////////////
    ///         RESOURCE QUERY          ///
    ///////////////////////////////////////

    /// @brief 버텍스 버퍼의 레이아웃 핸들로 버텍스 레이아웃을 반환합니다.
    [[nodiscard]] const VertexLayout& GetVertexLayout(VertexLayoutHandle _vlh);

    /// @brief 버텍스 버퍼의 레이아웃을 반환합니다.
    [[nodiscard]] const VertexLayout& GetVertexLayout(VertexBufferHandle _vbh);

    /// @brief 버텍스 버퍼의 생성 정보를 반환합니다.
    [[nodiscard]] VertexBufferInfo GetVertexBufferInfo(VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼의 생성 정보를 반환합니다.
    [[nodiscard]] IndexBufferInfo GetIndexBufferInfo(IndexBufferHandle _ibh);

    /// @brief 상수 버퍼의 생성 정보를 반환합니다.
    [[nodiscard]] ConstantBufferInfo GetConstantBufferInfo(ConstantBufferHandle _cbh);

    /// @brief 텍스처의 생성 정보를 반환합니다.
    [[nodiscard]] TextureInfo GetTextureInfo(TextureHandle _texh);

    /// @brief 프레임 버퍼의 생성 정보를 반환합니다.
    [[nodiscard]] FrameBufferInfo GetFrameBufferInfo(FrameBufferHandle _fbh);

    /// @brief 셰이더의 생성 정보를 반환합니다.
    [[nodiscard]] ShaderInfo GetShaderInfo(ShaderHandle _sh);

    /// @brief 프로그램의 생성 정보를 반환합니다.
    [[nodiscard]] ProgramInfo GetProgramInfo(ProgramHandle _ph);

    /// @brief 텍스처의 특정 MIP 레벨 해상도를 반환합니다.
    /// @param _texh 텍스처 핸들
    /// @param _mipIndex 조회할 MIP 레벨 (기본: 0)
    /// @return 해당 MIP 레벨의 width, height, depth
    [[nodiscard]] TextureResolution GetTextureResolution(
        TextureHandle _texh,
        Int32         _mipIndex = 0);

    /// @brief 백 버퍼의 해상도를 반환합니다.
    [[nodiscard]] Vec2i GetBackBufferResolution() const;

    /// @brief 프레임 버퍼에 첨부된 렌더 타겟 텍스처를 반환합니다.
    /// @param _fbh 프레임 버퍼 핸들. k_nullHandle 전달 시 백 버퍼의 렌더 타겟 텍스처를 반환합니다.
    /// @param _renderTargetIndex 렌더 타겟 슬롯 인덱스 (기본: 0)
    /// @return 렌더 타겟 텍스처 핸들 참조
    /// @note 렌더 타겟이 없으면 assertion이 발생합니다.
    ///       반환된 핸들은 프레임 버퍼가 소유합니다. DestroyTexture로 파괴하지 마십시오.
    [[nodiscard]] const TextureHandle& GetRenderTargetTexture(
        FrameBufferHandle _fbh,
        Int32             _renderTargetIndex = 0);

    /// @brief 프레임 버퍼에 첨부된 깊이-스텐실 텍스처를 반환합니다.
    /// @param _fbh 프레임 버퍼 핸들 (k_nullHandle 불가)
    /// @return 깊이-스텐실 텍스처 핸들 참조
    /// @note 깊이-스텐실이 없으면 assertion이 발생합니다.
    ///       반환된 핸들은 프레임 버퍼가 소유합니다. DestroyTexture로 파괴하지 마십시오.
    [[nodiscard]] const TextureHandle& GetDepthStencilTexture(FrameBufferHandle _fbh);

    /// @brief 프로그램에서 특정 셰이더 스테이지의 셰이더 핸들을 반환합니다.
    /// @param _ph 프로그램 핸들
    /// @param _stage 조회할 셰이더 스테이지
    /// @return 해당 스테이지의 셰이더 핸들
    /// @note 해당 스테이지에 셰이더가 없으면 assertion이 발생합니다.
    ///       반환된 핸들은 프로그램이 소유합니다. DestroyShader로 파괴하지 마십시오.
    [[nodiscard]] ShaderHandle GetShader(
        ProgramHandle _ph,
        eShader       _stage);

    ///////////////////////////////////////
    ///      RENDER TARGET OPERATIONS   ///
    ///////////////////////////////////////

    /// @brief 렌더 타겟을 지정된 색상으로 초기화합니다.
    /// @param _fbh 대상 프레임 버퍼 핸들. k_nullHandle 전달 시 백 버퍼를 클리어합니다.
    /// @param _color 초기화할 RGBA 색상
    /// @param _renderTargetIndex 초기화할 렌더 타겟 슬롯 인덱스 (기본: 0)
    void ClearColor(
        FrameBufferHandle _fbh,
        RGBA              _color,
        Int32             _renderTargetIndex = 0);

    /// @brief 깊이 버퍼 및/또는 스텐실 버퍼를 초기화합니다.
    /// @param _fbh 대상 프레임 버퍼 핸들 (k_nullHandle 불가. 백 버퍼는 깊이-스텐실이 없습니다.)
    /// @param _bClearDepth true: 깊이 버퍼를 _depth 값으로 초기화합니다.
    /// @param _bClearStencil true: 스텐실 버퍼를 _stencil 값으로 초기화합니다.
    /// @param _depth 깊이 초기화 값 (0.0 ~ 1.0, 기본: 1.0)
    /// @param _stencil 스텐실 초기화 값 (기본: 0)
    void ClearDepthStencil(
        FrameBufferHandle _fbh,
        bool              _bClearDepth,
        bool              _bClearStencil,
        float             _depth   = 1.f,
        UInt8             _stencil = 0);

    ///////////////////////////////////////
    ///       PIPELINE STATE BINDING    ///
    ///////////////////////////////////////

    /// @brief 버텍스 버퍼를 파이프라인에 바인딩합니다.
    /// @param _vbhOrInvalid 바인딩할 버텍스 버퍼 핸들. k_nullHandle 전달 시 바인딩을 해제합니다.
    /// @param _stream 버텍스 스트림 인덱스
    /// @note 버퍼 전체를 기본 오프셋(0)과 전체 버텍스 수로 바인딩합니다.
    void SetVertexBuffer(
        VertexBufferHandle _vbhOrInvalid,
        Int32              _stream);

    /// @brief 버텍스 버퍼를 범위를 지정하여 파이프라인에 바인딩합니다.
    /// @param _vbhOrInvalid 바인딩할 버텍스 버퍼 핸들
    /// @param _stream
    /// @param _numVerts 드로우 콜에 사용할 버텍스 수
    /// @param _vertOffset 버퍼 시작으로부터의 오프셋 (버텍스 단위)
    void SetVertexBuffer(
        VertexBufferHandle _vbhOrInvalid,
        Int32              _stream,
        Int32              _numVerts,
        Int32              _vertOffset);

    void SetVertexBuffer(
        VertexBufferHandle _vbhOrInvalid,
        Int32              _stream,
        Int32              _numVerts,
        Int32              _vertOffset,
        VertexLayoutHandle _vlh);

    void SetInstanceBuffer(
        VertexBufferHandle _vbhOrInvalid);

    void SetInstanceBuffer(
        VertexBufferHandle _vbhOrInvalid,
        Int32              _numInstances,
        Int32              _instanceOffset);

    /// @brief 인덱스 버퍼를 파이프라인에 바인딩합니다.
    /// @param _ibhOrInvalid 바인딩할 인덱스 버퍼 핸들. k_nullHandle 전달 시 바인딩을 해제합니다.
    void SetIndexBuffer(IndexBufferHandle _ibhOrInvalid);

    /// @brief 인덱스 버퍼를 범위를 지정하여 파이프라인에 바인딩합니다.
    /// @param _ibhOrInvalid 바인딩할 인덱스 버퍼 핸들
    /// @param _numIdxes 드로우 콜에 사용할 인덱스 수
    /// @param _idxOffset 버퍼 시작으로부터의 오프셋 (인덱스 단위)
    void SetIndexBuffer(
        IndexBufferHandle _ibhOrInvalid,
        Int32             _numIdxes,
        Int32             _idxOffset);

    /// @brief 상수 버퍼를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _cbh 바인딩할 상수 버퍼 핸들. k_nullHandle 전달 시 슬롯의 바인딩을 해제합니다.
    /// @param _shader 바인딩 대상 셰이더 스테이지
    /// @param _slot 상수 버퍼 슬롯 인덱스 (0 ~ k_maxNumConstantBufferSlots - 1)
    void SetConstantBuffer(
        ConstantBufferHandle _cbh,
        eShader              _shader,
        Int32                _slot);

    /// @brief 텍스처(SRV)를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _texh 바인딩할 텍스처 핸들. k_nullHandle 전달 시 슬롯의 바인딩을 해제합니다.
    ///             WriteOnly 텍스처는 SRV가 없어 바인딩되지 않습니다.
    /// @param _shader 바인딩 대상 셰이더 스테이지
    /// @param _slot SRV 슬롯 인덱스 (0 ~ k_maxNumShaderResourceViewSlots - 1)
    void SetTexture(
        TextureHandle _texh,
        eShader       _shader,
        Int32         _slot);

    /// @brief 샘플러 상태를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _state 적용할 샘플러 상태
    /// @param _shader 바인딩 대상 셰이더 스테이지
    /// @param _slot 샘플러 슬롯 인덱스 (0 ~ k_maxNumSamplerSlots - 1)
    void SetSamplerState(
        Flags<eSamplerFlags> _state,
        eShader              _shader,
        Int32                _slot);

    /// @brief 특정 슬롯의 샘플러 Border 색상을 설정합니다.
    /// @param _shader 셰이더 스테이지
    /// @param _slot 색상을 변경할 샘플러 슬롯 인덱스
    /// @param _rgba Border 색상. eTextureAddressMode::Border 사용 시에만 효과가 있습니다.
    void SetBorderColor(
        eShader _shader,
        Int32   _slot,
        RGBA    _rgba);

    /// @brief 래스터라이저, 깊이 테스트, 블렌딩의 통합 렌더 상태를 설정합니다.
    /// @param _flags 적용할 렌더 상태. 미리 정의된 RenderState::k_default, k_depthTest 등을 활용할 수 있습니다.
    /// @note 변경된 필드만 Dirty로 표시되어 Submit 시 최소한의 D3D11 상태만 갱신됩니다.
    void SetRenderFlags(
        Flags<eRenderFlags> _flags);

    /// @brief 특정 렌더 타겟 슬롯의 블렌드 상태를 설정합니다.
    /// @param _flags 적용할 블렌드 상태. 미리 정의된 BlendState::k_alphaBlend 등을 활용할 수 있습니다.
    /// @param _renderTargetIndex 대상 렌더 타겟 슬롯 인덱스 (0 ~ k_maxNumRenderTargetViewSlots - 1)
    void SetBlendFlags(
        Flags<eBlendFlags> _flags,
        Int32              _renderTargetIndex);

    /// @brief 블렌드 팩터 색상을 설정합니다.
    /// @param _rgba eBlend::BlendFactor / InvBlendFactor 사용 시 곱해지는 색상 팩터
    void SetBlendFactor(RGBA _rgba);

    /// @brief 스텐실 테스트 상태를 설정합니다.
    /// @param _fStencil 전면(Front-face) 스텐실 동작
    /// @param _bStencil 후면(Back-face) 스텐실 동작
    /// @param _stencilRef 스텐실 비교 참조 값 (기본: 0)
    void SetStencilFlags(
        Flags<eStencilFlags> _fStencil,
        Flags<eStencilFlags> _bStencil,
        UInt8                _stencilRef = 0);

    /// @brief 그래픽스 셰이더 프로그램을 파이프라인에 바인딩합니다.
    /// @param _ph 바인딩할 프로그램 핸들. k_nullHandle 전달 시 바인딩을 해제합니다.
    void SetProgram(ProgramHandle _ph);

    /// @brief 컴퓨트 셰이더 프로그램을 파이프라인에 바인딩합니다.
    /// @param _ph 바인딩할 프로그램 핸들. k_nullHandle 전달 시 바인딩을 해제합니다.
    void SetComputeProgram(ProgramHandle _ph);

    /// @brief 렌더링 결과를 출력할 프레임 버퍼를 설정합니다.
    /// @param _fbh 대상 프레임 버퍼 핸들. k_nullHandle 전달 시 백 버퍼에 렌더링합니다.
    void SetFrameBuffer(FrameBufferHandle _fbh);

    /// @brief 뷰포트를 설정합니다.
    /// @param _vp 뷰포트 구조체 (x, y, w, h)
    void SetViewport(Viewport _vp);

    /// @brief 뷰포트를 개별 인자로 설정합니다.
    /// @param _x, _y 뷰포트 좌상단 좌표
    /// @param _width, _height 뷰포트 크기
    void SetViewport(
        UInt16 _x,
        UInt16 _y,
        UInt16 _width,
        UInt16 _height);

    /// @brief 시저 테스트 영역을 설정합니다.
    /// @param _rect 시저 영역 구조체 (left, top, right, bottom)
    /// @note RenderState에서 bScissor = true로 설정해야 적용됩니다.
    void SetScissorRect(Rect _rect);

    /// @brief 시저 테스트 영역을 개별 인자로 설정합니다.
    /// @param _left, _top, _right, _bottom 시저 영역 좌표
    void SetScissorRect(
        UInt16 _left,
        UInt16 _top,
        UInt16 _right,
        UInt16 _bottom);

    ///////////////////////////////////////
    ///            DRAW CALL            ///
    ///////////////////////////////////////

    /// @brief 드로우 콜 제출 시 사용할 추가 파라미터를 설정합니다.
    /// @param _type 파라미터 타입 (StartVertexLocation, StartIndexLocation, BaseVertexLocation)
    /// @param _value 파라미터 값
    void SetSubmitParameter(
        eSubmitParameterType _type,
        Int32                _value);

    /// @brief 드로우 콜 없이 현재 Dirty 상태를 즉시 D3D11에 적용합니다.
    /// @note 일반적으로 Submit이 내부적으로 호출하므로 직접 호출할 필요가 없습니다.
    ///       상태 적용 후 내부 상태를 검사하거나 컴퓨트 셰이더 디스패치 전에 활용할 수 있습니다.
    void Flush();

    /// @brief 현재 바인딩된 리소스와 상태로 드로우 콜을 제출합니다.
    /// @note 인덱스 버퍼가 바인딩된 경우 DrawIndexed, 아닌 경우 Draw를 호출합니다.
    ///       호출 전 SetVertexBuffer, SetProgram, SetFrameBuffer, SetViewport 등이 바인딩되어 있어야 합니다.
    void Submit();

    /// @brief 현재 바인딩된 리소스와 상태로 컴퓨트 셰이더 디스패치를 제출합니다.
    /// @param _numThreadGroupsX X 방향으로 디스패치할 스레드 그룹 수
    /// @param _numThreadGroupsY Y 방향으로 디스패치할 스레드 그룹 수
    /// @param _numThreadGroupsZ Z 방향으로 디스패치할 스레드 그룹 수
    void Dispatch(
        Int32 _numThreadGroupsX,
        Int32 _numThreadGroupsY,
        Int32 _numThreadGroupsZ);

private:
    Graphics(HWND _hWnd, Int32 _width, Int32 _height, bool _bAllowTearing);
    ~Graphics();

    static void CreateInstance(HWND _hwnd, Int32 _width, Int32 _height, bool _bAllowTearing);
    static void DestroyInstance();

    void BindConstantBuffers_(eShader _stage);
    void BindShaderResourceViews_(eShader _shader);
    void BindSamplerStates_(eShader _shader);
    void BindPipeline_();

    // event handlers
    void OnResize_(Int32 _width, Int32 _height);
    void OnWindowResizedEvent_(const WindowResizedEvent& _event);
    void OnWindowMaximizeEvent_(const WindowMaximizedEvent& _event);

    // native resource factory
    [[nodiscard]] HardwareTextureRef CreateHardwareTexture_(Int32 _width, Int32 _height, Int32 _depth, eTextureFormat _format, Int32 _arraySize, eTextureType _type, Flags<eTextureCreateFlags> _flags, Span<const MemoryView> _initDataOrEmpty) const;
    [[nodiscard]] FrameBuffer        CreateFrameBuffer_(Span<const TextureHandle> _attachments, bool _bDestroyTexture);

    [[nodiscard]] ShaderHandle       GetOrCreateShader_(MemoryView _byteCode);
    [[nodiscard]] VertexLayoutHandle GetOrCreateVertexLayout_(const VertexLayout& _vl);

    // buffer utils
    [[nodiscard]] MutableMemoryView BeginUpdateBuffer_(const HardwareBuffer& _buffer) const;
    void                            EndUpdateBuffer_(const HardwareBuffer& _buffer) const;
    void                            UpdateBuffer_(const HardwareBuffer& _buffer, MemoryView _uploadData, Int32 _offset, bool _bDiscard);
    [[nodiscard]] MemoryView        BeginReadBuffer_(const HardwareBuffer& _buffer) const;
    void                            EndReadBuffer_(const HardwareBuffer& _buffer) const;
    Int32                           ReadBuffer_(const HardwareBuffer& _buffer, MemoryView _dst) const;
    void                            CopyBuffer_(const HardwareBuffer& _dstBuffer, Int32 _dstOffset, const HardwareBuffer& _srcBuffer, Int32 _srcOffset, Int32 _byteWidth) const;

    // texture utils
    void                     UpdateTexture_(const HardwareTextureRef& _dstTexture, Int32 _dstMipIndex, Int32 _dstArrayIndex, Int32 _dstX, Int32 _dstY, Int32 _dstZ, MemoryView _uploadData, Int32 _uploadWidth, Int32 _uploadHeight, Int32 _uploadDepth, Int32 _uploadDataRowPitch, Int32 _uploadDataSlicePitch) const;
    [[nodiscard]] MemoryView BeginReadTexture_(const HardwareTextureRef& _texture, Int32 _mipIndex, Int32 _arrayIndex) const;
    void                     EndReadTexture_(const HardwareTextureRef& _texture, Int32 _mipIndex, Int32 _arrayIndex) const;
    [[nodiscard]] Int32      ReadTexture_(const HardwareTextureRef& _texture, Int32 _mipIndex, Int32 _arrayIndex, MemoryView _dst, Int32 _dstRowPitch, Int32 _dstSlicePitch) const;
    void                     CopyTexture_(const HardwareTextureRef& _dstTexture, Int32 _dstMipIndex, Int32 _dstArrayIndex, Int32 _dstX, Int32 _dstY, Int32 _dstZ, const HardwareTextureRef& _srcTexture, Int32 _srcMipIndex, Int32 _srcArrayIndex, Int32 _srcX, Int32 _srcY, Int32 _srcZ, Int32 _srcWidthOrZero, Int32 _srcHeightOrZero, Int32 _srcDepthOrZero) const;

    [[nodiscard]] HardwareBuffer CreateHardwareBuffer_(Int32 _byteWidth, eBufferType _type, MemoryView _initDataOrEmpty) const;
    [[nodiscard]] StorageBuffer  CreateStorageBuffer_(Int32 _byteWidth, Int32 _stride, eStorageBufferType _type, eStorageBufferDataType _dataType, Flags<eStorageBufferCreateFlags> _flags, bool _bAppendConsumer, bool _bCounter, MemoryView _initDataOrEmpty) const;

    // d3d11 cache
    [[nodiscard]] ID3D11InputLayout*       GetOrCreateD3D11InputLayoutFromCurrentBind_();
    [[nodiscard]] ID3D11RasterizerState*   GetOrCreateD3D11RasterizerStateFromCurrentBind_();
    [[nodiscard]] ID3D11BlendState*        GetOrCreateD3D11BlendStateFromCurrentBind_();
    [[nodiscard]] ID3D11DepthStencilState* GetOrCreateD3D11DepthStencilStateFromCurrentBind_();
    [[nodiscard]] ID3D11SamplerState*      GetOrCreateD3D11SamplerState_(Flags<eSamplerFlags> _state, RGBA _rgba);

    // upload/download resource cache
    [[nodiscard]] ID3D11Buffer* GetOrCreateUploadBuffer_(Int32 _byteWidth);

    // misc
    [[nodiscard]] UInt32 QueryMaxMsaaQualityLevel_(DXGI_FORMAT _format, Int32 _numSamples) const;

    // d3d11 core objects
    ID3D11Device*        m_pd3d11Device        = nullptr;
    ID3D11DeviceContext* m_pd3d11DeviceContext = nullptr;
    IDXGISwapChain1*     m_pdxgiSwapchain      = nullptr;
    bool                 m_bAllowTearing       = false;

    // resource pools
    ResourcePool<VertexBuffer>       m_vertexBuffers  = {};
    ResourcePool<IndexBuffer>        m_indexBuffers   = {};
    ResourcePool<HardwareTextureRef> m_textures       = {};
    ResourcePool<FrameBuffer>        m_frameBuffers   = {};
    ResourcePool<ShaderRef>          m_shaders        = {};
    ResourcePool<VertexLayoutRef>    m_vertexLayouts  = {};
    ResourcePool<ConstantBuffer>     m_cbuffers       = {};
    ResourcePool<Program>            m_programs       = {};
    ResourcePool<StorageBuffer>      m_storageBuffers = {};

    // resource caches
    CASHashMap<UInt64, VertexLayoutHandle> m_vlCache     = {};
    CASHashMap<UInt64, ShaderHandle>       m_shaderCache = {};

    // d3d11 caches
    CASHashMap<UInt64, ID3D11InputLayout*>       m_d3d11InputLayoutCache       = {};
    CASHashMap<UInt32, ID3D11RasterizerState*>   m_d3d11RasterizerStateCache   = {};
    CASHashMap<UInt64, ID3D11BlendState*>        m_d3d11BlendStateCache        = {};
    CASHashMap<UInt64, ID3D11DepthStencilState*> m_d3d11DepthStencilStateCache = {};
    CASHashMap<UInt64, ID3D11SamplerState*>      m_d3d11SamplerStateCache      = {};

    // vertex stream
    StreamBinds m_streamBinds = {};
    Int32       m_numVerts    = 0;

    // index buffer
    IndexBufferHandle m_ibhBind        = k_nullHandle;
    Int32             m_numIndicesBind = 0;
    Int32             m_ibOffsetBind   = 0;

    // instance buffer
    VertexBufferHandle m_instanceBind       = k_nullHandle;
    Int32              m_numInstancesBind   = 0;
    Int32              m_instanceOffsetBind = 0;

    // read resources
    EnumArray<eShader, ConstantBufferBinds> m_cbufferBinds = {};
    EnumArray<eShader, TextureBinds>        m_textureBinds = {};
    EnumArray<eShader, SamplerBinds>        m_samplerBinds = {};

    // write resources
    FrameBufferHandle                      m_fbhBind         = k_nullHandle;
    ProgramHandle                          m_phBind          = k_nullHandle;
    ProgramHandle                          m_computePhBind   = k_nullHandle;
    Viewport                               m_vpBind          = {};
    Rect                                   m_scissorRectBind = {};
    EnumArray<eSubmitParameterType, Int32> m_submitParams    = {};

    // render state
    Flags<eRenderFlags> m_renderFlagsBind = {};

    // blend state
    Array<Flags<eBlendFlags>, k_maxNumRenderTargetViewSlots> m_blendFlagsBinds = {};
    RGBA                                                     m_blendFactorBind = {};

    // stencil states
    Flags<eStencilFlags> m_fstencilBind   = eStencilFlags::None;
    Flags<eStencilFlags> m_bstencilBind   = eStencilFlags::None;
    UInt8                m_stencilRefBind = 0;

    // dirty flags
    Flags<ePipelineDirtyFlags> m_dirtyFlags = ePipelineDirtyFlags::All;

    // stream bound (TODO: fix me -> more efficient way to track vertex layout for each stream)
    Array<VertexLayoutHandle, k_maxNumVertexStream> m_vlhStreamBound  = {};
    Int32                                           m_numStreamsBound = 0;

    // back buffer
    Vec2i             m_backBufferRes  = {};
    TextureHandle     m_backBufferTexh = k_nullHandle;
    FrameBufferHandle m_backBufferFbh  = k_nullHandle;

    // upload resource cache
    ID3D11Buffer* m_pUploadBufferCache     = nullptr;
    Int32         m_pUploadBufferCacheSize = 0;

    inline static Graphics* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] inline Graphics& GetGraphics()
{
    return Graphics::GetInstance();
}

}   // namespace Jam

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