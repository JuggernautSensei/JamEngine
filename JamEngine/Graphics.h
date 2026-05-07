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

/// @brief D3D11 기반 그래픽스 백엔드.
///
/// 버퍼, 텍스처, 셰이더 등 GPU 리소스의 생성/파괴를 담당하며,
/// Dirty Flag 기반의 파이프라인 상태 관리와 드로우 콜 제출 기능을 제공합니다.
class Graphics
{
    enum class ePipelineDirtyFlags : UInt64
    {
        None = 0,

        PrimitiveTopology = 1ull << 0,
        VertexBuffer      = 1ull << 1,
        InstanceBuffer    = 1ull << 2,
        InputLayout       = 1ull << 3,
        IndexBuffer       = 1ull << 4,

        Program        = 1ull << 5,
        ComputeProgram = 1ull << 6,

        VSConstantBuffer = 1ull << 7,
        PSConstantBuffer = 1ull << 8,
        GSConstantBuffer = 1ull << 9,
        HSConstantBuffer = 1ull << 10,
        DSConstantBuffer = 1ull << 11,
        CSConstantBuffer = 1ull << 12,

        VSShaderResourceView = 1ull << 13,
        PSShaderResourceView = 1ull << 14,
        GSShaderResourceView = 1ull << 15,
        HSShaderResourceView = 1ull << 16,
        DSShaderResourceView = 1ull << 17,
        CSShaderResourceView = 1ull << 18,

        PSUnorderedAccessView = 1ull << 19,
        CSUnorderedAccessView = 1ull << 20,

        VSSamplerState = 1ull << 21,
        PSSamplerState = 1ull << 22,
        GSSamplerState = 1ull << 23,
        HSSamplerState = 1ull << 24,
        DSSamplerState = 1ull << 25,
        CSSamplerState = 1ull << 26,

        RasterizerState   = 1ull << 27,
        BlendState        = 1ull << 28,
        DepthStencilState = 1ull << 29,
        Viewport          = 1ull << 30,
        ScissorRect       = 1ull << 31,
        FrameBuffer       = 1ull << 32,

        End = FrameBuffer,
        All = 0xFFFFFFFFFFFFFFFF,

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
    };

    struct Resource
    {
        UInt32          value = kInvalidHandleValue;
        eShaderResource type  = eShaderResource::Texture;
    };

    struct ReadBindings
    {
        ReadBindings()
        {
            binder.fill(nullptr);
            resources.fill({});
        }

        Array<ID3D11ShaderResourceView*, kMaxNumShaderResourceViewSlots> binder     = {};
        Array<Resource, kMaxNumShaderResourceViewSlots>                  resources  = {};
        Int32                                                            dirtyBegin = kMaxNumShaderResourceViewSlots;
        Int32                                                            dirtyEnd   = 0;
    };

    struct ReadWriteBindings
    {
        ReadWriteBindings()
        {
            binder.fill(nullptr);
            resources.fill({});
        }

        Array<ID3D11UnorderedAccessView*, kMaxNumUnorderedAccessViewSlots> binder     = {};
        Array<Resource, kMaxNumUnorderedAccessViewSlots>                   resources  = {};
        Int32                                                              dirtyBegin = kMaxNumUnorderedAccessViewSlots;
        Int32                                                              dirtyEnd   = 0;
        UInt64                                                             useMask    = 0;
    };

    struct ConstantBufferBindings
    {
        ConstantBufferBindings()
        {
            binder.fill(nullptr);
            cbhs.fill(kInvalidHandle);
        }

        Array<ID3D11Buffer*, kMaxNumConstantBufferSlots>        binder     = {};
        Array<ConstantBufferHandle, kMaxNumConstantBufferSlots> cbhs       = {};
        Int32                                                   dirtyBegin = kMaxNumConstantBufferSlots;
        Int32                                                   dirtyEnd   = 0;
    };

    struct SamplerBindings
    {
        struct Sampler
        {
            Flags<eSamplerFlags> flags = {};
            RGBA                 rgba  = {};
        };

        SamplerBindings()
        {
            binder.fill(nullptr);
            samplers.fill({});
        }

        Array<ID3D11SamplerState*, kMaxNumSamplerSlots> binder     = {};
        Array<Sampler, kMaxNumSamplerSlots>             samplers   = {};
        Int32                                           dirtyBegin = kMaxNumSamplerSlots;
        Int32                                           dirtyEnd   = 0;
    };

    struct VertexBufferBindings
    {
        VertexBufferBindings()
        {
            binder.fill(nullptr);
            strides.fill(0);
            offsets.fill(0);
            vbhs.fill(kInvalidHandle);
        }

        Array<ID3D11Buffer*, kMaxNumVertexBufferSlots> binder  = {};
        Array<UInt32, kMaxNumVertexBufferSlots>        strides = {};
        Array<UInt32, kMaxNumVertexBufferSlots>        offsets = {};

        Array<VertexBufferHandle, kMaxNumVertexBufferSlots> vbhs          = {};
        Array<VertexLayoutHandle, kMaxNumVertexBufferSlots> vlhs          = {};
        Int32                                               numBuffers    = 0;
        Int32                                               numVerts      = 0;
        Int32                                               numAvailVerts = 0;
    };

    struct InstanceBufferBinding
    {
        ID3D11Buffer* binder = nullptr;
        UInt32        stride = 0;
        UInt32        offset = 0;

        VertexBufferHandle vbh               = kInvalidHandle;
        Int32              numInstances      = 0;
        Int32              numAvailInstances = 0;
    };

    struct IndexBufferBinding
    {
        ID3D11Buffer* binder = nullptr;
        DXGI_FORMAT   format = DXGI_FORMAT_R16_UINT;
        UInt32        offset = 0;

        IndexBufferHandle ibh             = kInvalidHandle;
        Int32             numIndices      = 0;
        Int32             numAvailIndices = 0;
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
    /// @param _vertData 초기 버텍스 데이터
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
    /// @param _stride 각 인스턴스 데이터의 크기 (바이트 단위, 16의 배수여야 함)
    /// @return 생성된 인스턴스 버퍼 핸들
    [[nodiscard]] VertexBufferHandle CreateInstanceBuffer(
        Int32 _numInstances,
        Int32 _stride);

    /// @brief 수정 불가능한(Immutable) 인덱스 버퍼를 생성합니다.
    /// @param _indexData 초기 인덱스 데이터 (비어 있으면 안 됩니다)
    /// @param _bUseU32 true: 32비트(UInt32) 인덱스 포맷, false: 16비트(UInt16) 인덱스 포맷
    /// @return 생성된 인덱스 버퍼 핸들
    /// @note 생성 후에는 데이터를 수정할 수 없는 대신 GPU 접근 성능이 뛰어납니다.
    ///       정적인 메시 인덱스에 사용하는 것을 권장합니다.
    [[nodiscard]] IndexBufferHandle CreateIndexBuffer(
        MemoryView _indexData,
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

    /// @brief Raw Storage 버퍼를 생성합니다.
    /// @param _byteWidth 버퍼 크기 (바이트 단위, 4의 배수여야 합니다)
    /// @param _flags 생성 플래그.
    /// @param _initDataOrEmpty 초기화 데이터. 비어 있으면 초기화되지 않은 버퍼가 생성됩니다.
    /// @return 생성된 스토리지 버퍼 핸들
    /// @note 생성된 버퍼는 이후 SetTexture가 아닌, 스토리지 버퍼 바인딩 API(해당 기능 제공 시)를 통해 사용합니다.
    [[nodiscard]] StorageBufferHandle CreateRawBuffer(
        Int32                            _byteWidth,
        Flags<eStorageBufferCreateFlags> _flags           = eStorageBufferCreateFlags::None,
        MemoryView                       _initDataOrEmpty = {});

    /// @brief Structured Storage 버퍼를 생성합니다.
    /// @param _numElements 요소 개수.
    /// @param _stride 요소 하나의 크기 (바이트 단위).
    /// @param _bAppendConsume true면 Append/Consume UAV로 생성합니다.
    /// @param _bCounter true면 카운터를 포함하는 UAV로 생성합니다.
    /// @param _flags 생성 플래그.
    /// @param _initDataOrEmpty
    /// @return 생성된 스토리지 버퍼 핸들
    /// @note `_bAppendConsume`와 `_bCounter` 조합은 D3D11 UAV 생성 제약을 따릅니다.
    [[nodiscard]] StorageBufferHandle CreateStructuredBuffer(
        Int32                            _numElements,
        Int32                            _stride,
        bool                             _bAppendConsume  = false,
        bool                             _bCounter        = false,
        Flags<eStorageBufferCreateFlags> _flags           = eStorageBufferCreateFlags::None,
        MemoryView                       _initDataOrEmpty = {});

    /// @brief Typed Storage 버퍼를 생성합니다.
    /// @param _numElements 요소 개수.
    /// @param _dataType 요소 데이터 타입(포맷).
    /// @param _flags 생성 플래그.
    /// @param _initDataOrEmpty
    /// @return 생성된 스토리지 버퍼 핸들
    [[nodiscard]] StorageBufferHandle CreateTypedBuffer(
        Int32                            _numElements,
        eStorageBufferData               _dataType,
        Flags<eStorageBufferCreateFlags> _flags           = eStorageBufferCreateFlags::None,
        MemoryView                       _initDataOrEmpty = {});

    /// @brief Readback 버퍼를 생성합니다.
    /// @param _byteWidth 버퍼 크기 (바이트 단위)
    /// @param _initDataOrEmpty
    /// @return 생성된 스토리지 버퍼 핸들
    [[nodiscard]] StorageBufferHandle CreateReadbackBuffer(
        Int32      _byteWidth,
        MemoryView _initDataOrEmpty = {});

    /// @brief 버텍스 레이아웃을 생성합니다.
    /// @param _vl 버텍스 레이아웃 설명
    /// @return 생성된 버텍스 레이아웃 핸들
    /// @note 버텍스 버퍼를 GPU에 바인딩할 때 필요한 입력 레이아웃입니다. 동일한 레이아웃을 여러 버텍스 버퍼에서 공유할 수 있습니다.
    ///       버텍스 버퍼를 생성할 때, 자동으로 생성되기 때문에, 수동으로 생성할 필요는 없습니다.
    ///       그러나, 세밀한 조정이 필요한 경우, 수동으로 관리할 수 있습니다.
    [[nodiscard]] VertexLayoutHandle CreateVertexLayout(
        const VertexLayout& _vl);

    /// @brief 다이나믹 버텍스 버퍼에 데이터를 업로드합니다.
    /// @param _bh 대상 버텍스 버퍼 핸들 (Dynamic 버퍼여야 합니다)
    /// @param _writeData 업로드할 데이터
    /// @param _offset 버퍼 내 오프셋 (바이트 단위, 기본: 0)
    /// @param _bDiscard true: 버퍼 전체를 무효화한 뒤 새 데이터를 씁니다.
    ///                  false: 기존 데이터를 유지하면서 지정 범위만 부분 업데이트합니다.
    /// @note 부분 업데이트가 더 빠를거라는 보장은 없습니다. 실제 성능은 드라이버 구현에 따라 달라질 수 있습니다.
    void UpdateBuffer(
        AnyBufferHandle _bh,
        MemoryView      _writeData,
        Int32           _offset   = 0,
        bool            _bDiscard = true);

    /// @brief 다이나믹 버텍스 버퍼에 데이터를 업로드하기 위한 메모리 뷰를 얻습니다.
    /// @param _bh 대상 버텍스 버퍼 핸들 (Dynamic 버퍼여야 합니다)
    /// @return 버퍼 데이터를 쓸 수 있는 MutableMemoryView
    /// @note BeginUpdateBuffer로 얻은 메모리 뷰에 데이터를 쓴 후에는 반드시 EndUpdateBuffer를 호출하여 업데이트를 완료해야 합니다. 이 함수는 내부적으로 필요한 동기화 작업을 수행하여 GPU가 업데이트된 데이터를 안전하게 사용할 수 있도록 합니다.
    ///       이 함수는 낮은 수준의 권한을 제공하므로, 사용 시 주의가 필요합니다. 이 함수보다는 UpdateBuffer를 사용하는 것을 권장합니다.
    MutableMemoryView BeginUpdateBuffer(
        AnyBufferHandle _bh) const;

    /// @brief BeginUpdateBuffer로 시작한 버퍼 업데이트를 완료합니다.
    /// @param _bh 대상 버퍼 핸들
    /// @note BeginUpdateBuffer로 얻은 메모리 뷰에 대한 모든 쓰기가 완료된 후에 호출해야 합니다. 이 함수는 내부적으로 필요한 동기화 작업을 수행하여 GPU가 업데이트된 데이터를 안전하게 사용할 수 있도록 합니다.
    ///       해당 함수는 낮은 수준의 권한을 제공하므로, 사용 시 주의가 필요합니다. 이 함수보다는 UpdateBuffer를 사용하는 것을 권장합니다.
    void EndUpdateBuffer(
        AnyBufferHandle _bh) const;

    /// @brief 버퍼간 데이터 복사를 수행합니다.
    /// @param _dst 대상 버퍼 핸들
    /// @param _dstOffset 대상 버퍼 내 오프셋 (바이트 단위)
    /// @param _src 원본 버퍼 핸들
    /// @param _srcOffset 원본 버퍼 내 오프셋 (바이트 단위)
    /// @param _byteWidthOrZero 복사할 바이트 수. 0이면, src 버퍼의 끝까지 복사합니다.
    void CopyBuffer(
        AnyBufferHandle _dst,
        Int32           _dstOffset,
        AnyBufferHandle _src,
        Int32           _srcOffset,
        Int32           _byteWidthOrZero = 0) const;

    /// @brief 리드백 버퍼에서 데이터를 읽어옵니다.
    /// @param _sbh 읽을 스토리지 버퍼 핸들 (Readback 버퍼여야 합니다)
    /// @param _dst 데이터를 받을 메모리 블록. 읽을 크기가 버퍼 크기를 초과하면, 클립되어 읽힙니다.
    /// @return 읽은 바이트 수를 반환합니다.
    Int32 ReadBuffer(
        StorageBufferHandle _sbh,
        MutableMemoryView   _dst);

    ////////// TEXTURE /////////

    /// @brief ImageContainer로부터 텍스처를 생성합니다.
    /// @param _container 원본 이미지 컨테이너 (2D, 3D, Cubemap 모두 지원)
    /// @param _bMutable true로 설정하면 생성 후에도 CPU에서 데이터를 수정할 수 있습니다. 기본적으로는 Immutable 텍스처로 생성됩니다.
    /// @return 생성된 텍스처 핸들
    /// @note 파일에서 읽어온 텍스처를 GPU에 올릴 때 사용합니다.
    ///       미리 생성된 MIP 체인이 있으면 그대로 업로드합니다.
    [[nodiscard]] TextureHandle CreateTexture(
        const ImageContainer& _container,
        bool                  _bMutable = false);

    /// @brief 2D 텍스처를 생성합니다.
    /// @param _width 텍스처 너비 (픽셀 단위, 1 이상)
    /// @param _height 텍스처 높이 (픽셀 단위, 1 이상)
    /// @param _format 텍스처 포맷
    /// @param _bHasMips 밉맵 레벨을 생성할지 여부 (기본: false).
    /// @param _numLayers 텍스처 배열 크기 (기본: 1)
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
        bool                       _bHasMips        = false,
        Int32                      _numLayers       = 1,
        Flags<eTextureCreateFlags> _flags           = eTextureCreateFlags::None,
        Span<const MemoryView>     _initDataOrEmpty = {});

    /// @brief 큐브맵 텍스처를 생성합니다.
    /// @param _width 각 큐브 면의 너비 (픽셀 단위, 1 이상)
    /// @param _height 각 큐브 면의 높이 (픽셀 단위, 1 이상, 일반적으로 _width와 동일)
    /// @param _format 텍스처 포맷
    /// @param _bHasMips 밉맵 레벨을 생성할지 여부 (기본: false).
    /// @param _cubeCount 큐브맵 개수 (기본: 1). 내부 배열 크기는 _cubeCount * 6이 됩니다.
    /// @param _flags 텍스처 생성 플래그 (기본: None)
    /// @param _initDataOrEmpty 면별 초기화 데이터 배열. 비어 있으면 초기화되지 않은 텍스처가 생성됩니다.
    ///                         제공 시 배열 크기는 cubeCount * 6 * numMips여야 합니다.
    /// @return 생성된 큐브맵 텍스처 핸들
    [[nodiscard]] TextureHandle CreateTextureCube(
        Int32                      _width,
        Int32                      _height,
        eTextureFormat             _format,
        bool                       _bHasMips        = false,
        Int32                      _cubeCount       = 1,
        Flags<eTextureCreateFlags> _flags           = eTextureCreateFlags::None,
        Span<const MemoryView>     _initDataOrEmpty = {});

    /// @brief 3D 볼륨 텍스처를 생성합니다.
    /// @param _width 텍스처 너비 (픽셀 단위, 1 이상)
    /// @param _height 텍스처 높이 (픽셀 단위, 1 이상)
    /// @param _depth 텍스처 깊이 (슬라이스 수, 1 이상)
    /// @param _format 텍스처 포맷
    /// @param _bHasMips
    /// @param _texFlags 텍스처 생성 플래그 (기본: None)
    /// @param _initDataOrEmpty MIP 레벨별 초기화 데이터 배열. 비어 있으면 초기화되지 않은 텍스처가 생성됩니다.
    /// @return 생성된 3D 텍스처 핸들
    [[nodiscard]] TextureHandle CreateTexture3D(
        Int32                      _width,
        Int32                      _height,
        Int32                      _depth,
        eTextureFormat             _format,
        bool                       _bHasMips        = false,
        Flags<eTextureCreateFlags> _texFlags        = eTextureCreateFlags::None,
        Span<const MemoryView>     _initDataOrEmpty = {});

    /// @brief 2D 텍스처의 특정 부분 영역(Sub-region)을 업데이트합니다.
    /// @param _texh 업데이트할 텍스처 핸들
    /// @param _mip 업데이트할 MIP 레벨 인덱스
    /// @param _layer 업데이트할 배열 인덱스
    /// @param _x 업데이트 영역의 시작 X 좌표
    /// @param _y 업데이트 영역의 시작 Y 좌표
    /// @param _widthOrZero 업데이트할 영역의 너비 (0이면 텍스처 너비 전체를 사용)
    /// @param _heightOrZero 업데이트할 영역의 높이 (0이면 텍스처 높이 전체를 사용)
    /// @param _data 업데이트에 사용할 소스 데이터 메모리 뷰
    /// @param _rowPitch 소스 데이터의 한 행(Row) 바이트 크기
    void UpdateTexture2D(
        TextureHandle _texh,
        Int32         _mip,
        Int32         _layer,
        Int32         _x,
        Int32         _y,
        Int32         _widthOrZero,
        Int32         _heightOrZero,
        MemoryView    _data,
        Int32         _rowPitch) const;

    /// @brief 큐브맵 텍스처의 특정 면(Face) 내 부분 영역을 업데이트합니다.
    /// @param _texh 업데이트할 큐브맵 텍스처 핸들
    /// @param _mip 업데이트할 MIP 레벨 인덱스
    /// @param _face 업데이트할 면 인덱스 (eCubeFace 열거형)
    /// @param _layer 업데이트할 배열 인덱스
    /// @param _x 업데이트 영역의 시작 X 좌표
    /// @param _y 업데이트 영역의 시작 Y 좌표
    /// @param _widthOrZero 업데이트할 영역의 너비
    /// @param _heightOrZero 업데이트할 영역의 높이
    /// @param _data 업데이트에 사용할 소스 데이터 메모리 뷰
    /// @param _rowPitch 소스 데이터의 한 행(Row) 바이트 크기
    void UpdateTextureCube(
        TextureHandle _texh,
        Int32         _mip,
        eCubeFace     _face,
        Int32         _layer,
        Int32         _x,
        Int32         _y,
        Int32         _widthOrZero,
        Int32         _heightOrZero,
        MemoryView    _data,
        Int32         _rowPitch) const;

    /// @brief 3D 볼륨 텍스처의 특정 부분 박스(Sub-box) 영역을 업데이트합니다.
    /// @param _texh 업데이트할 3D 텍스처 핸들
    /// @param _mip 업데이트할 MIP 레벨 인덱스
    /// @param _x 업데이트 영역의 시작 X 좌표
    /// @param _y 업데이트 영역의 시작 Y 좌표
    /// @param _z 업데이트 영역의 시작 Z 좌표
    /// @param _widthOrZero 업데이트할 영역의 너비
    /// @param _heightOrZero 업데이트할 영역의 높이
    /// @param _depthOrZero 업데이트할 영역의 깊이
    /// @param _data 업데이트에 사용할 소스 데이터 메모리 뷰
    /// @param _rowPitch 소스 데이터 한 행(Row)의 바이트 크기
    /// @param _depthPitch 소스 데이터 한 슬라이스(Slice)의 바이트 크기
    void UpdateTexture3D(
        TextureHandle _texh,
        Int32         _mip,
        Int32         _x,
        Int32         _y,
        Int32         _z,
        Int32         _widthOrZero,
        Int32         _heightOrZero,
        Int32         _depthOrZero,
        MemoryView    _data,
        Int32         _rowPitch,
        Int32         _depthPitch) const;

    /// @brief 텍스처의 지정된 영역을 다른 텍스처의 지정된 위치로 GPU 복사합니다.
    /// @param _dstTexh 대상 텍스처 핸들
    /// @param _dstMip 대상 MIP 레벨
    /// @param _dstLayer 대상 배열/슬라이스 인덱스
    /// @param _dstX, _dstY, _dstZ 대상 오프셋 (픽셀 단위)
    /// @param _srcTexh 원본 텍스처 핸들
    /// @param _srcMip 원본 MIP 레벨
    /// @param _srcLayer 원본 배열/슬라이스 인덱스
    /// @param _srcX, _srcY, _srcZ 원본 영역의 시작 오프셋 (픽셀 단위)
    /// @param _srcWidthOrZero 복사할 너비. 0이면 원본 텍스처 전체 너비를 사용합니다.
    /// @param _srcHeightOrZero 복사할 높이. 0이면 원본 텍스처 전체 높이를 사용합니다.
    /// @param _srcDepthOrZero 복사할 깊이. 0이면 원본 텍스처 전체 깊이를 사용합니다.
    void CopyTexture(
        TextureHandle _dstTexh,
        Int32         _dstMip,
        Int32         _dstLayer,
        Int32         _dstX,
        Int32         _dstY,
        Int32         _dstZ,
        TextureHandle _srcTexh,
        Int32         _srcMip,
        Int32         _srcLayer,
        Int32         _srcX,
        Int32         _srcY,
        Int32         _srcZ,
        Int32         _srcWidthOrZero,
        Int32         _srcHeightOrZero,
        Int32         _srcDepthOrZero) const;

    /// @brief 텍스처의 특정 MIP/슬라이스 데이터를 CPU 메모리로 읽어옵니다.
    /// @param _texh 읽을 텍스처 핸들.
    /// @param _srcMip 원본 MIP 레벨.
    /// @param _srcLayer 원본 배열/슬라이스 인덱스.
    /// @param _dst 픽셀 데이터를 받을 메모리. 크기는 포맷/해상도에 따라 충분해야 합니다.
    /// @return 실제로 복사된 바이트 수
    /// @note GPU-CPU 동기화가 발생할 수 있습니다.
    Int32 ReadTexture(
        TextureHandle     _texh,
        Int32             _srcMip,
        Int32             _srcLayer,
        MutableMemoryView _dst) const;

    /// @brief 텍스처 전체를 ImageContainer로 캡처합니다.
    /// @param _texh 캡처할 텍스처 핸들
    /// @return 캡처된 이미지 컨테이너 (파일 저장 등에 활용 가능)
    /// @note GPU-CPU 동기화가 발생합니다. 성능이 중요한 경로에서는 사용하지 마십시오.
    [[nodiscard]] ImageContainer Capture(TextureHandle _texh);

    ////////// FRAME BUFFER /////////

    /// @brief 프레임 버퍼를 생성합니다.
    /// @param _attachments 프레임 버퍼에 첨부할 텍스처 핸들 배열.
    ///                     깊이 포맷 텍스처는 자동으로 깊이-스텐실 어태치먼트로 인식됩니다.
    ///                     렌더 타겟은 최대 kMaxNumRenderTargetViewSlots개, 깊이-스텐실은 최대 1개까지 가능합니다.
    /// @param _bDestroyTexture true: 프레임 버퍼 파괴 시 첨부된 텍스처도 함께 파괴됩니다.
    ///                         false: 텍스처를 외부에서 독립적으로 관리합니다.
    ///                         텍스처를 다른 용도로도 사용한다면 반드시 false로 설정하십시오.
    /// @return 생성된 프레임 버퍼 핸들
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        Span<const Attachment> _attachments,
        bool                   _bDestroyTexture = false);

    /// @brief 단일 텍스처로 프레임 버퍼를 생성합니다.
    /// @param _texh 첨부할 텍스처 핸들 (렌더 타겟 또는 깊이-스텐실)
    /// @param _bDestroyTexture true: 프레임 버퍼 파괴 시 텍스처도 함께 파괴됩니다.
    /// @return 생성된 프레임 버퍼 핸들
    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(
        TextureHandle _texh,
        bool          _bDestroyTexture = false);

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
    /// @param _vsh 버텍스 셰이더 핸들 (kInvalidHandle 불가)
    /// @param _psh 픽셀 셰이더 핸들 (kInvalidHandle 불가)
    /// @param _gsh 지오메트리 셰이더 핸들. 지오메트리 셰이더를 사용하지 않으면 kInvalidHandle을 전달하십시오.
    /// @param _hsh 헐 셰이더 핸들. 테셀레이션을 사용하지 않으면 kInvalidHandle을 전달하십시오.
    /// @param _dsh 도메인 셰이더 핸들. 테셀레이션을 사용하지 않으면 kInvalidHandle을 전달하십시오.
    /// @param _bDestroyShader true: 프로그램 파괴 시 소유권을 가져가 셰이더도 함께 파괴합니다.
    ///                        false: 셰이더의 참조 카운트만 증가시킵니다. 셰이더를 별도로 파괴해야 합니다.
    /// @return 생성된 프로그램 핸들
    /// @note HS와 DS는 반드시 함께 제공하거나 둘 다 kInvalidHandle이어야 합니다.
    [[nodiscard]] ProgramHandle CreateProgram(
        ShaderHandle _vsh,
        ShaderHandle _psh,
        ShaderHandle _gsh,
        ShaderHandle _hsh,
        ShaderHandle _dsh,
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
    void DestroyVertexBuffer(
        VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼를 파괴합니다.
    /// @param _ibh 파괴할 인덱스 버퍼 핸들
    /// @note 편의 매크로 JAM_DESTROY_INDEX_BUFFER 사용을 권장합니다.
    void DestroyIndexBuffer(
        IndexBufferHandle _ibh);

    /// @brief 버텍스 레이아웃을 파괴합니다.
    /// @param _vlh 파괴할 버텍스 레이아웃 핸들
    /// @note  수동으로 관리할 때만 파괴할 필요가 있습니다.
    ///        편의 매크로 JAM_DESTROY_VERTEX_LAYOUT 사용을 권장합니다.
    void DestroyVertexLayout(
        VertexLayoutHandle _vlh);

    /// @brief 상수 버퍼를 파괴합니다.
    /// @param _cbh 파괴할 상수 버퍼 핸들
    /// @note 편의 매크로 JAM_DESTROY_CONSTANT_BUFFER 사용을 권장합니다.
    void DestroyConstantBuffer(
        ConstantBufferHandle _cbh);

    /// @brief 스토리지 버퍼를 파괴합니다.
    /// @param _sbh 파괴할 스토리지 버퍼 핸들
    void DestroyStorageBuffer(
        StorageBufferHandle _sbh);

    /// @brief 텍스처를 파괴합니다.
    /// @param _texh 파괴할 텍스처 핸들
    /// @note 내부 참조 카운트가 0이 될 때 실제 D3D11 리소스가 해제됩니다.
    ///       FrameBuffer에 의해 관리되는 텍스처는 직접 파괴하지 마십시오.
    ///       편의 매크로 JAM_DESTROY_TEXTURE 사용을 권장합니다.
    void DestroyTexture(
        TextureHandle _texh);

    /// @brief 프레임 버퍼를 파괴합니다.
    /// @param _fbh 파괴할 프레임 버퍼 핸들
    /// @note _bDestroyTexture = true로 생성된 경우 첨부된 텍스처도 함께 파괴됩니다.
    ///       편의 매크로 JAM_DESTROY_FRAME_BUFFER 사용을 권장합니다.
    void DestroyFrameBuffer(
        FrameBufferHandle _fbh);

    /// @brief 셰이더를 파괴합니다.
    /// @param _sh 파괴할 셰이더 핸들
    /// @note 내부 참조 카운트가 0이 될 때 실제 D3D11 셰이더가 해제됩니다.
    ///       GetShader로 얻은 핸들은 파괴하지 마십시오. Program이 소유권을 가집니다.
    ///       편의 매크로 JAM_DESTROY_SHADER 사용을 권장합니다.
    void DestroyShader(
        ShaderHandle _sh);

    /// @brief 프로그램을 파괴합니다.
    /// @param _ph 파괴할 프로그램 핸들
    /// @note 내부적으로 각 셰이더의 참조 카운트를 감소시킵니다.
    ///       편의 매크로 JAM_DESTROY_PROGRAM 사용을 권장합니다.
    void DestroyProgram(
        ProgramHandle _ph);

    ///////////////////////////////////////
    ///          DEBUG NAMING           ///
    ///////////////////////////////////////

    /// @brief 버텍스 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _vbh 이름을 설정할 버텍스 버퍼 핸들
    /// @param _name 설정할 이름 (PIX, RenderDoc 등 GPU 디버거에서 표시됩니다)
    void SetVertexBufferName(
        VertexBufferHandle _vbh,
        StringView         _name);

    /// @brief 인덱스 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _ibh 이름을 설정할 인덱스 버퍼 핸들
    /// @param _name 설정할 이름
    void SetIndexBufferName(
        IndexBufferHandle _ibh,
        StringView        _name);

    /// @brief 상수 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _cbh 이름을 설정할 상수 버퍼 핸들
    /// @param _name 설정할 이름
    void SetConstantBufferName(
        ConstantBufferHandle _cbh,
        StringView           _name);

    /// @brief 스토리지 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _sbh 이름을 설정할 스토리지 버퍼 핸들
    /// @param _name 설정할 이름
    void SetStorageBufferName(
        StorageBufferHandle _sbh,
        StringView          _name);

    /// @brief 텍스처에 디버깅용 이름을 설정합니다.
    /// @param _texh 이름을 설정할 텍스처 핸들
    /// @param _name 설정할 이름
    /// @note 텍스처 리소스와 SRV 모두에 이름이 설정됩니다.
    void SetTextureName(
        TextureHandle _texh,
        StringView    _name);

    /// @brief 프레임 버퍼에 디버깅용 이름을 설정합니다.
    /// @param _fbh 이름을 설정할 프레임 버퍼 핸들
    /// @param _name 설정할 이름
    /// @note RTV와 DSV 모두에 이름이 설정됩니다.
    void SetFrameBufferName(
        FrameBufferHandle _fbh,
        StringView        _name);

    /// @brief 셰이더에 디버깅용 이름을 설정합니다.
    /// @param _sh 이름을 설정할 셰이더 핸들
    /// @param _name 설정할 이름
    void SetShaderName(
        ShaderHandle _sh,
        StringView   _name);

    //////////////////////////////////////
    ///         RESOURCE Info          ///
    //////////////////////////////////////

    /// @brief 버텍스 버퍼의 레이아웃 핸들을 반환합니다.
    [[nodiscard]] VertexLayoutHandle GetVertexLayout(
        VertexBufferHandle _vbh);

    /// @brief 버텍스 레이아웃을 반환합니다.
    [[nodiscard]] const VertexLayout& GetVertexLayout(
        VertexLayoutHandle _vlh);

    /// @brief 버텍스 버퍼 정보를 함께 반환합니다.
    [[nodiscard]] VertexBufferInfo GetVertexBufferInfo(
        VertexBufferHandle _vbh);

    /// @brief 인덱스 버퍼 정보를 반환합니다.
    [[nodiscard]] IndexBufferInfo GetIndexBufferInfo(
        IndexBufferHandle _ibh);

    /// @brief 상수 버퍼 정보를 반환합니다.
    [[nodiscard]] ConstantBufferInfo GetConstantBufferInfo(
        ConstantBufferHandle _cbh);

    /// @brief 스토리지 버퍼 정보를 반환합니다.
    [[nodiscard]] StorageBufferInfo GetStorageBufferInfo(
        StorageBufferHandle _sbh);

    /// @brief 텍스처 정보를 반환합니다.
    [[nodiscard]] TextureInfo GetTextureInfo(
        TextureHandle _texh);

    /// @brief 프레임 버퍼 정보를 반환합니다.
    /// @param _fbh 프레임 버퍼 핸들. kInvalidHandle 전달 시 백 버퍼 정보를 반환합니다.
    [[nodiscard]] FrameBufferInfo GetFrameBufferInfo(
        FrameBufferHandle _fbh);

    /// @brief 프레임 버퍼에 첨부된 텍스처 핸들을 반환합니다.
    /// @param _fbh 프레임 버퍼 핸들. kInvalidHandle 전달 시 백 버퍼 정보를 반환합니다.
    /// @param _slot 반환할 첨부 인덱스. 깊이 버퍼를 얻어내고 싶다면 kDepthStencilAttachmentIndex를 전달하십시오.
    [[nodiscard]] Attachment GetFrameBufferAttachment(
        FrameBufferHandle _fbh,
        Int32             _slot);

    /// @brief 프레임 버퍼의 해상도를 반환합니다.
    /// @param _fbh 프레임 버퍼 핸들. kInvalidHandle 전달 시 백 버퍼 해상도를 반환합니다.
    /// @note 일반적으로 프레임 버퍼를 이루는 텍스처들의 해상도는 동일합니다.
    ///       깊이버퍼가 있으면 깊이버퍼의 해상도를 리턴하고, 없으면 0번 렌더 타겟 텍스처의 해상도를 리턴합니다.
    [[nodiscard]] Vec3i GetFrameBufferResolution(
        FrameBufferHandle _fbh);

    /// @brief 백 버퍼의 해상도를 반환합니다.
    [[nodiscard]] Vec2i GetBackBufferResolution() const;

    /// @brief 셰이더 정보를 반환합니다.
    [[nodiscard]] ShaderInfo GetShaderInfo(
        ShaderHandle _sh);

    /// @brief 프로그램 정보를 반환합니다.
    [[nodiscard]] ProgramInfo GetProgramInfo(
        ProgramHandle _ph);

    /// @brief 그래픽스 API의 현재 상태에 대한 통계를 반환합니다.
    [[nodiscard]] const GraphicsStats& GetStats() const;

    ///////////////////////////////////////
    ///      RENDER TARGET OPERATIONS   ///
    ///////////////////////////////////////

    /// @brief 렌더 타겟을 지정된 색상으로 초기화합니다.
    /// @param _fbh 대상 프레임 버퍼 핸들. kInvalidHandle 전달 시 백 버퍼를 클리어합니다.
    /// @param _color 초기화할 RGBA 색상
    /// @param _renderTargetIndex 초기화할 렌더 타겟 슬롯 인덱스 (기본: 0)
    void ClearRenderTarget(
        FrameBufferHandle _fbh,
        RGBA              _color,
        Int32             _renderTargetIndex = 0);

    /// @brief 깊이 버퍼 및/또는 스텐실 버퍼를 초기화합니다.
    /// @param _fbh 대상 프레임 버퍼 핸들 (kInvalidHandle 불가. 백 버퍼는 깊이-스텐실이 없습니다.)
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

    /// @brief 버텍스 버퍼를 범위를 지정하여 파이프라인에 바인딩합니다.
    /// @param _vbhOrInvalid 바인딩할 버텍스 버퍼 핸들 또는 kInvalidHandle (바인딩 해제)
    /// @param _vertOffset 버텍스 버퍼 시작으로부터의 오프셋 (버텍스 단위, 기본값: 0)
    /// @param _numVerts 드로우 콜에 사용할 버텍스 수. kUseAll 전달 시 버텍스 버퍼 전체를 사용합니다.
    void SetVertexBuffer(
        VertexBufferHandle _vbhOrInvalid,
        Int32              _vertOffset = 0,
        Int32              _numVerts   = kAll);

    /// @brief 버텍스 버퍼와 인스턴스 버퍼를 범위를 지정하여 파이프라인에 바인딩합니다.
    /// @param _vbhOrInvalid 버텍스 버퍼 핸들 또는 kInvalidHandle (버텍스 버퍼 바인딩 해제)
    /// @param _vertOffset 버텍스 버퍼 시작으로부터의 오프셋 (버텍스 단위, 기본값: 0)
    /// @param _numVerts 드로우 콜에 사용할 버텍스 수. kUseAll 전달 시 버텍스 버퍼 전체를 사용합니다.
    /// @param _instanceVbhOrInvalid 인스턴스 버퍼 핸들 또는 kInvalidHandle (인스턴스 버퍼 바인딩 해제)
    /// @param _instanceOffset 인스턴스 버퍼 시작으로부터의 오프셋 (인스턴스 단위, 기본값: 0)
    /// @param _numInstances 드로우 콜에 사용할 인스턴스 수. kUseAll 전달 시 인스턴스 버퍼 전체를 사용합니다.
    void SetVertexBuffer(
        VertexBufferHandle _vbhOrInvalid,
        Int32              _vertOffset,
        Int32              _numVerts,
        VertexBufferHandle _instanceVbhOrInvalid,
        Int32              _instanceOffset = 0,
        Int32              _numInstances   = kAll);

    /// @brief 여러 버텍스 스트림을 범위를 지정하여 파이프라인에 바인딩합니다.
    /// @param _streams 바인딩할 버텍스 스트림 배열. 각 스트림은 버텍스 버퍼 핸들과 버텍스 수, 오프셋을 포함합니다.
    /// @param _numVerts 드로우 콜에 사용할 버텍스 수. kUseAll 전달 시 버텍스 버퍼 전체를 사용합니다.
    void SetVertexBuffers(
        Span<const VertexStream> _streams,
        Int32                    _numVerts = kAll);

    /// @brief 여러 버텍스 스트림과 인스턴스 버퍼를 범위를 지정하여 파이프라인에 바인딩합니다.
    /// @param _streams 바인딩할 버텍스 스트림 배열. 각 스트림은 버텍스 버퍼 핸들과 버텍스 수, 오프셋을 포함합니다.
    /// @param _numVerts 드로우 콜에 사용할 버텍스 수. kUseAll 전달 시 각 스트림의 버텍스 버퍼 전체를 사용합니다.
    /// @param _instanceVbhOrInvalid 인스턴스 버퍼 핸들. kInvalidHandle 전달 시 인스턴스 버퍼 바인딩이 해제됩니다.
    /// @param _instanceOffset 인스턴스 버퍼 시작으로부터의 오프셋 (인스턴스 단위, 기본값: 0)
    /// @param _numInstances 드로우 콜에 사용할 인스턴스 수. kUseAll 전달 시 인스턴스 버퍼 전체를 사용합니다.
    void SetVertexBuffers(
        Span<const VertexStream> _streams,
        Int32                    _numVerts,
        VertexBufferHandle       _instanceVbhOrInvalid,
        Int32                    _instanceOffset = 0,
        Int32                    _numInstances   = kAll);

    /// @brief 인덱스 버퍼를 범위를 지정하여 파이프라인에 바인딩합니다.
    /// @param _ibhOrInvalid 바인딩할 인덱스 버퍼 핸들
    /// @param _indexOffset 버퍼 시작으로부터의 오프셋 (인덱스 단위)
    /// @param _numIndices 드로우 콜에 사용할 인덱스 수. kUseAll 전달 시 인덱스 버퍼 전체를 사용합니다.
    void SetIndexBuffer(
        IndexBufferHandle _ibhOrInvalid,
        Int32             _indexOffset = 0,
        Int32             _numIndices  = kAll);

    /// @brief 상수 버퍼를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _cbh 바인딩할 상수 버퍼 핸들. kInvalidHandle 전달 시 슬롯의 바인딩을 해제합니다.
    /// @param _shader 바인딩 대상 셰이더 스테이지
    /// @param _slot 상수 버퍼 슬롯 인덱스 (0 ~ kMaxNumConstantBufferSlots - 1)
    void SetConstantBuffer(
        ConstantBufferHandle _cbh,
        eShader              _shader,
        Int32                _slot);

    /// @brief 텍스처를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _texh 바인딩할 텍스처 핸들. kInvalidHandle 전달 시 슬롯의 바인딩을 해제합니다.
    /// @param _shader 바인딩 대상 셰이더 스테이지
    /// @param _slot 슬롯 인덱스
    /// @param _access 셰이더가 텍스처에 접근하는 모드. 기본값은 Read입니다. .
    /// @note ReadWrite로 바인딩하려면 텍스처는 반드시 ShaderReadWrite 플래그로 생성되어야 하며, ReadWrite 바인딩은 Pixel/Compute 셰이더에서만 허용됩니다.
    void SetTexture(
        TextureHandle   _texh,
        eShader         _shader,
        Int32           _slot,
        eResourceAccess _access = eResourceAccess::Read);

    /// @brief 스토리지 버퍼를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _sbh 바인딩할 스토리지 버퍼 핸들. kInvalidHandle 전달 시 슬롯의 바인딩을 해제합니다.
    /// @param _shader 바인딩 대상 셰이더 스테이지
    /// @param _slot 슬롯 인덱스
    /// @param _access 스토리지 버퍼에 대한 셰이더 접근 모드. 기본값은 Read입니다.
    /// @note ReadWrite로 바인딩하려면 버퍼는 반드시 ShaderReadWrite 플래그로 생성되어야 하며, ReadWrite 바인딩은 Pixel/Compute 셰이더에서만 허용됩니다.
    void SetBuffer(
        StorageBufferHandle _sbh,
        eShader             _shader,
        Int32               _slot,
        eResourceAccess     _access = eResourceAccess::Read);

    /// @brief 샘플러 상태를 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _flags 적용할 샘플러 상태
    /// @param _shader 바인딩 대상 셰이더 스테이지
    /// @param _slot 샘플러 슬롯 인덱스 (0 ~ kMaxNumSamplerSlots - 1)
    void SetSampler(
        Flags<eSamplerFlags> _flags,
        eShader              _shader,
        Int32                _slot);

    /// @brief 샘플러 상태와 함께 경계 색상을 특정 셰이더 스테이지의 슬롯에 바인딩합니다.
    /// @param _flags 적용할 샘플러 상태
    /// @param _rgba eSamplerFlags::BorderColor 사용 시 샘플링에 활용할 RGBA 경계 색상
    /// @param _shader 바인딩 대상 셰이더 스테이지
    /// @param _slot 샘플러 슬롯 인덱스 (0 ~ kMaxNumSamplerSlots - 1)
    void SetSampler(
        Flags<eSamplerFlags> _flags,
        RGBA                 _rgba,
        eShader              _shader,
        Int32                _slot);

    /// @brief 래스터라이저, 깊이 테스트, 블렌딩의 통합 렌더 상태를 설정합니다.
    /// @param _flags 적용할 렌더 상태. 미리 정의된 RenderState::kDefault, kDepthTest 등을 활용할 수 있습니다.
    /// @note 변경된 필드만 Dirty로 표시되어 Submit 시 최소한의 D3D11 상태만 갱신됩니다.
    void SetRenderFlags(
        Flags<eRenderFlags> _flags);

    /// @brief 특정 렌더 타겟 슬롯의 블렌드 상태를 설정합니다.
    /// @param _flags 적용할 블렌드 상태. 미리 정의된 BlendState::kAlphaBlend 등을 활용할 수 있습니다.
    /// @param _renderTargetIndex 대상 렌더 타겟 슬롯 인덱스 (0 ~ kMaxNumRenderTargetViewSlots - 1)
    void SetBlendFlags(
        Flags<eBlendFlags> _flags,
        Int32              _renderTargetIndex);

    /// @brief 블렌드 팩터 색상을 설정합니다.
    /// @param _rgba eBlend::BlendFactor / InvBlendFactor 사용 시 곱해지는 색상 팩터
    void SetBlendFactor(
        RGBA _rgba);

    /// @brief 스텐실 테스트 상태를 설정합니다.
    /// @param _fStencil 전면(Front-face) 스텐실 동작
    /// @param _bStencil 후면(Back-face) 스텐실 동작
    /// @param _stencilRef 스텐실 비교 참조 값 (기본: 0)
    void SetStencilFlags(
        Flags<eStencilFlags> _fStencil,
        Flags<eStencilFlags> _bStencil,
        UInt8                _stencilRef = 0);

    /// @brief 그래픽스 셰이더 프로그램을 파이프라인에 바인딩합니다.
    /// @param _ph 바인딩할 프로그램 핸들. kInvalidHandle 전달 시 바인딩을 해제합니다.
    void SetProgram(
        ProgramHandle _ph);

    /// @brief 컴퓨트 셰이더 프로그램을 파이프라인에 바인딩합니다.
    /// @param _ph 바인딩할 프로그램 핸들. kInvalidHandle 전달 시 바인딩을 해제합니다.
    void SetComputeProgram(
        ProgramHandle _ph);

    /// @brief 렌더링 결과를 출력할 프레임 버퍼를 설정합니다.
    /// @param _fbh 대상 프레임 버퍼 핸들. kInvalidHandle 전달 시 백 버퍼에 렌더링합니다.
    void SetFrameBuffer(
        FrameBufferHandle _fbh);

    /// @brief 뷰포트를 설정합니다.
    /// @param _vp 뷰포트 구조체 (x, y, w, h)
    void SetViewport(
        Viewport _vp);

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
    void SetScissorRect(
        Rect _rect);

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
        eSubmitParameter _type,
        Int32            _value);

    /// @brief 드로우 콜 없이 현재 Dirty 상태를 즉시 D3D11에 적용합니다.
    /// @note 일반적으로 Submit이 내부적으로 호출하므로 직접 호출할 필요가 없습니다.
    ///       상태 적용 후 내부 상태를 검사하거나 컴퓨트 셰이더 디스패치 전에 활용할 수 있습니다.
    void Touch();

    /// @brief 파이프리인을 초기 상태로 리셋합니다. 모든 바인딩과 상태가 해제되고 기본값으로 돌아갑니다.
    /// @note 해당 명령어는 즉시 적용됩니다.
    ///       오버헤드가 존재하기 때문에, 빈번한 호출은 추천하지 않습니다.
    void Reset();

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

    // buffer utils
    [[nodiscard]] HardwareBuffer        CreateHardwareBuffer_(Int32 _byteWidth, eBuffer _type, MemoryView _initDataOrEmpty) const;
    [[nodiscard]] StorageBuffer         CreateStorageBuffer_(Int32 _byteWidth, Int32 _stride, eStorageBuffer _type, eStorageBufferData _dataType, Flags<eStorageBufferCreateFlags> _flags, bool _bAppendConsumer, bool _bCounter, MemoryView _initDataOrEmpty) const;
    [[nodiscard]] const HardwareBuffer& GetHardwareBufferFromAnyBufferHandle_(AnyBufferHandle _handle) const;
    [[nodiscard]] ID3D11Buffer*         GetOrCreateUploadBuffer_(Int32 _byteWidth);
    [[nodiscard]] MutableMemoryView     BeginUpdateBuffer_(AnyBufferHandle _bh) const;
    void                                EndUpdateBuffer_(AnyBufferHandle _bh) const;
    void                                UpdateBuffer_(AnyBufferHandle _bh, MemoryView _uploadData, Int32 _offset, bool _bDiscard);
    void                                CopyBuffer_(AnyBufferHandle _dstBh, Int32 _dstOffset, AnyBufferHandle _srcBh, Int32 _srcOffset, Int32 _byteWidthOrZero) const;

    // texture utils
    [[nodiscard]] HardwareTextureRef CreateHardwareTexture_(Int32 _width, Int32 _height, Int32 _depth, eTextureFormat _format, bool _bHasMips, Int32 _numLayers, eTexture _type, Flags<eTextureCreateFlags> _flags, Span<const MemoryView> _initDataOrEmpty) const;
    void                             UpdateTexture_(TextureHandle _dstTexh, Int32 _dstMip, Int32 _dstLayer, Int32 _offsetX, Int32 _offsetY, Int32 _offsetZ, Int32 _widthOrZero, Int32 _heightOrZero, Int32 _depthOrZero, MemoryView _uploadData, Int32 _rowPitch, Int32 _slicePitch) const;
    [[nodiscard]] Int32              ReadTexture_(TextureHandle _texh, Int32 _mip, Int32 _layer, MutableMemoryView _dst) const;
    void                             CopyTexture_(TextureHandle _dstTexh, Int32 _dstMip, Int32 _dstLayer, Int32 _dstOffsetX, Int32 _dstOffsetY, Int32 _dstOffsetZ, TextureHandle _srcTexh, Int32 _srcMip, Int32 _srcLayer, Int32 _srcOffsetX, Int32 _srcOffsetY, Int32 _srcOffsetZ, Int32 _srcWidthOrZero, Int32 _srcHeightOrZero, Int32 _srcDepthOrZero) const;

    // frame buffer utils
    [[nodiscard]] FrameBuffer CreateFrameBuffer_(Span<const Attachment> _attachments, bool _bDestroyTexture);

    // resource cache
    [[nodiscard]] ShaderHandle             GetOrCreateShader_(MemoryView _byteCode);
    [[nodiscard]] VertexLayoutHandle       GetOrCreateVertexLayout_(const VertexLayout& _vl);
    [[nodiscard]] ID3D11InputLayout*       GetOrCreateD3D11InputLayoutFromCurrentBind_();
    [[nodiscard]] ID3D11RasterizerState*   GetOrCreateD3D11RasterizerStateFromCurrentBind_();
    [[nodiscard]] ID3D11BlendState*        GetOrCreateD3D11BlendStateFromCurrentBind_();
    [[nodiscard]] ID3D11DepthStencilState* GetOrCreateD3D11DepthStencilStateFromCurrentBind_();
    [[nodiscard]] ID3D11SamplerState*      GetOrCreateD3D11SamplerState_(Flags<eSamplerFlags> _state, RGBA _rgba);

    // misc
    void                 SetShaderResource_(UInt32 _handleValueOrNull, eShaderResource _type, eShader _shader, Int32 _slot, eResourceAccess _access);
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

    /////////// binding & binder ///////////

    // buffers
    VertexBufferBindings  m_vbBindings      = {};
    IndexBufferBinding    m_ibBinding       = {};
    InstanceBufferBinding m_instanceBinding = {};

    // input resources
    EnumArray<eShader, ReadBindings>           m_readBindings    = {};
    EnumArray<eShader, SamplerBindings>        m_samplerBindings = {};
    EnumArray<eShader, ConstantBufferBindings> m_cbufferBindings = {};

    ReadWriteBindings m_csReadWriteBindings = {};
    ReadWriteBindings m_psReadWriteBindings = {};

    // output resources
    FrameBufferHandle                  m_fbhBinding          = kInvalidHandle;
    Viewport                           m_vpBinding           = {};
    Rect                               m_scissorRectBinding  = {};
    ProgramHandle                      m_phBinding           = kInvalidHandle;
    EnumArray<eSubmitParameter, Int32> m_submitParamsBinding = {};
    ProgramHandle                      m_computePhBinding    = kInvalidHandle;

    // states
    Flags<eRenderFlags>                                     m_renderFlagsBinding = eRenderFlags::None;
    Array<Flags<eBlendFlags>, kMaxNumRenderTargetViewSlots> m_blendFlagsBindings = {};
    RGBA                                                    m_blendFactorBinding = {};
    Flags<eStencilFlags>                                    m_fstencilBinding    = eStencilFlags::None;
    Flags<eStencilFlags>                                    m_bstencilBinding    = eStencilFlags::None;
    UInt8                                                   m_stencilRefBinding  = 0;

    // dirty flags
    Flags<ePipelineDirtyFlags> m_dirtyFlags = ePipelineDirtyFlags::All;

    // misc
    GraphicsStats m_lastStats = {};
    GraphicsStats m_stats     = {};

    // back buffer
    TextureHandle     m_backBufferTexh       = kInvalidHandle;
    FrameBufferHandle m_backBufferFbh        = kInvalidHandle;
    Vec2i             m_backBufferResolution = {};

    // upload buffer cache
    ID3D11Buffer* m_pUploadBufferCache     = nullptr;
    Int32         m_pUploadBufferCacheSize = 0;

    friend class Application;
    inline static Graphics* s_pInstance = nullptr;
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
        (_handle) = Jam::kInvalidHandle;            \
    }                                               \
    JAM_END_MACRO_BLOCK

#define JAM_DESTROY_VERTEX_BUFFER(_handle)   JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyVertexBuffer)
#define JAM_DESTROY_INDEX_BUFFER(_handle)    JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyIndexBuffer)
#define JAM_DESTROY_CONSTANT_BUFFER(_handle) JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyConstantBuffer)
#define JAM_DESTROY_STORAGE_BUFFER(_handle)  JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyStorageBuffer)
#define JAM_DESTROY_PROGRAM(_handle)         JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyProgram)
#define JAM_DESTROY_TEXTURE(_handle)         JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyTexture)
#define JAM_DESTROY_FRAME_BUFFER(_handle)    JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyFrameBuffer)
#define JAM_DESTROY_SHADER(_handle)          JAM_DESTROY_GRAPHICS_HANDLE(_handle, GetGraphics().DestroyShader)