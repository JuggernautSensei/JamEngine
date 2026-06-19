#pragma once
#include "GraphicsBase.h"
#include "StringKeyedContainers.h"

namespace Jam
{

enum class eRenderGraphClearFlags : UInt32
{
    Resource = 1 << 0,
    Pass     = 1 << 1,
    All      = Resource | Pass,
};

struct AttachmentRG
{
    String name        = {};
    Int32  mip         = 0;
    Int32  layerOffset = 0;
    Int32  numLayers   = 1;
};

class RenderGraph
{
    enum class ePass
    {
        Utility,
        Render,
        Compute,
    };

    struct Slot
    {
        [[nodiscard]] bool operator==(const Slot& other) const
        {
            return stage == other.stage && slot == other.slot;
        }

        eShader stage = eShader::VertexShader;
        Int32   slot  = 0;
    };

    struct SlotHasher
    {
        [[nodiscard]] USizeT operator()(const Slot& _slot) const
        {
            Murmur3Hasher hasher;
            hasher.Mix(AsMemoryView(_slot.stage));
            hasher.Mix(AsMemoryView(_slot.slot));
            return hasher.Finish();
        }
    };

    struct ResourceBindCommand
    {
        String          name        = {};
        UInt32          handleValue = kInvalidHandleValue;
        eShaderResource type        = eShaderResource::Texture;
        Slot            slot        = {};
    };

    struct ConstantBufferBindCommand
    {
        String               name = {};
        ConstantBufferHandle cbh  = kInvalidHandle;
        Slot                 slot = {};
    };

    struct Sampler
    {
        [[nodiscard]] bool operator==(const Sampler& other) const
        {
            return flags == other.flags && rgba == other.rgba;
        }

        [[nodiscard]] bool operator!=(const Sampler& other) const
        {
            return !(*this == other);
        }

        Flags<eSamplerFlags> flags = {};
        RGBA                 rgba  = RGBA::kBlack;
    };

    struct SamplerFlagsBindCommand
    {
        Sampler state = {};
        Slot    slot  = {};
    };

    struct BlendFlagsBindCommand
    {
        Flags<eBlendFlags> state = {};
        Int32              slot  = 0;
    };

    struct RenderTargetClearCommand
    {
        RGBA  color = RGBA::kBlack;
        Int32 slot  = 0;
    };

    struct PassInfo
    {
        String name = {};
        ePass  type = ePass::Utility;

        // graphics
        String        programName = {};
        ProgramHandle ph          = kInvalidHandle;

        // compute
        String        computeProgramName = {};
        ProgramHandle computePh          = kInvalidHandle;

        // framebuffer
        String            frameBufferName = {};
        FrameBufferHandle fbh             = kInvalidHandle;

        // viewport
        Viewport viewport                             = {};
        bool     bUseFrameBufferResolutionForViewport = true;

        // scissor
        Rect scissorRect = {};

        // render state
        Flags<eRenderFlags>  renderFlags = {};
        Flags<eStencilFlags> fstencil    = {};
        Flags<eStencilFlags> bstencil    = {};
        UInt8                stencilRef  = 0;

        // blend
        Array<BlendFlagsBindCommand, kMaxNumRenderTargetViewSlots> blendFlagsBindCmds    = {};
        Int32                                                      numBlendFlagsBindCmds = 0;
        RGBA                                                       blendFactor           = RGBA::kTransparent;

        // clear
        Array<RenderTargetClearCommand, kMaxNumRenderTargetViewSlots> renderTargetClearCmds    = {};
        Int32                                                         numRenderTargetClearCmds = 0;
        bool                                                          bClearDepth              = false;
        float                                                         depthClearValue          = 1.f;
        bool                                                          bClearStencil            = false;
        UInt8                                                         stencilClearValue        = 0;

        // resource bindings
        HashMap<Slot, ConstantBufferBindCommand, SlotHasher> cbufferBindCmds   = {};
        HashMap<Slot, SamplerFlagsBindCommand, SlotHasher>   samplerBindCmds   = {};
        HashMap<Slot, ResourceBindCommand, SlotHasher>       readBindCmds      = {};
        HashMap<Slot, ResourceBindCommand, SlotHasher>       readWriteBindCmds = {};

        // 의존성 분석용
        StringHashSet<> readResources      = {};
        StringHashSet<> readWriteResources = {};
        StringHashSet<> writeResources     = {};

        Delegate<void()> executor = nullptr;

        // 최종 패스 또는 사이드이펙트 패스여야 의존성 컬링에서 살아남음
        bool bFinalPass  = false;   // 최종 패스의 경우 자체적인 프레임버퍼를 가지지 않음. Execute 과정에서 전달받은 프레임버퍼에 렌더링함.
        bool bSideEffect = false;
        bool bEnable     = true;
    };

    struct Pass
    {
        PassInfo* pDesc = nullptr;

        // 언바인드 목록
        Vector<Slot> readUnbindSlots      = {};
        Vector<Slot> readWriteUnbindSlots = {};
        bool         bUnbindFrameBuffer   = false;

        // 바인드 목록
        Vector<ResourceBindCommand>       readBindCmds      = {};
        Vector<ResourceBindCommand>       readWriteBindCmds = {};
        Vector<ConstantBufferBindCommand> cbufferBindCmds   = {};
        Vector<SamplerFlagsBindCommand>   samplerBindCmds   = {};
    };

    template<typename THandle>
    struct Resource
    {
        String  name          = {};
        THandle handle        = kInvalidHandle;
        bool    bHasOwnership = false;
    };

    struct TextureResource : Resource<TextureHandle>
    {
        eTexture             type        = eTexture::Texture2D;
        bool                 bAutoResize = false;
        float                resizeScale = 1.f;
        eTextureFormat       format      = eTextureFormat::Unknown;
        bool                 bHasMips    = false;
        Int32                numLayers   = 1;
        Flags<eTextureFlags> flags       = eTextureFlags::None;
    };

    struct FrameBufferResource : Resource<FrameBufferHandle>
    {
        Array<String, kMaxNumAttachmentSlots> attachments      = {};
        Int32                                 numRenderTargets = 0;
        bool                                  bHasDepthStencil = false;
        bool                                  bAutoResize      = false;
    };

    using ConstantBufferResource = Resource<ConstantBufferHandle>;
    using StorageBufferResource  = Resource<StorageBufferHandle>;
    using ShaderResource         = Resource<ShaderHandle>;
    using ProgramResource        = Resource<ProgramHandle>;

public:
    class UtilityPassBuilder
    {
    public:
        UtilityPassBuilder(RenderGraph* _pGraph, PassInfo* _pDesc);

        UtilityPassBuilder& Enable(bool _bEnable = true);
        UtilityPassBuilder& SetExecutor(const Delegate<void()>& _executor);

    private:
        RenderGraph* m_pGraph = nullptr;
        PassInfo*    m_pInfo  = nullptr;
    };

    class RenderPassBuilder
    {
    public:
        RenderPassBuilder(RenderGraph* _pGraph, PassInfo* _pDesc);

        RenderPassBuilder& Enable(bool _bEnable = true);
        RenderPassBuilder& AsFinalPass(bool _bEnable = true);
        RenderPassBuilder& AsSideEffect(bool _bEnable = true);

        RenderPassBuilder& SetProgram(StringView _name);
        RenderPassBuilder& SetFrameBuffer(StringView _name);
        RenderPassBuilder& SetViewport(Viewport _vp);
        RenderPassBuilder& SetFrameBufferResolutionForViewport();
        RenderPassBuilder& SetScissorRect(Rect _rect);

        RenderPassBuilder& SetRenderTargetClearCommand(RGBA _color, Int32 _slot);
        RenderPassBuilder& SetDepthClearCommand(float _value);
        RenderPassBuilder& SetStencilClearCommand(UInt8 _value);

        RenderPassBuilder& SetRenderFlags(Flags<eRenderFlags> _flags);
        RenderPassBuilder& SetStencilFlags(Flags<eStencilFlags> _fstencil, Flags<eStencilFlags> _bstencil, UInt8 _ref);
        RenderPassBuilder& SetBlendFlags(Flags<eBlendFlags> _flags, Int32 _slot);
        RenderPassBuilder& SetBlendFactor(RGBA _factor);

        RenderPassBuilder& SetTexture(StringView _name, eShader _stage, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        RenderPassBuilder& SetBuffer(StringView _name, eShader _stage, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        RenderPassBuilder& SetConstantBuffer(StringView _name, eShader _stage, Int32 _slot);
        RenderPassBuilder& SetSampler(Flags<eSamplerFlags> _state, eShader _stage, Int32 _slot);
        RenderPassBuilder& SetSampler(Flags<eSamplerFlags> _state, RGBA _borderColor, eShader _stage, Int32 _slot);

        RenderPassBuilder& SetExecutor(const Delegate<void()>& _executor);

    private:
        RenderGraph* m_pGraph = nullptr;
        PassInfo*    m_pInfo  = nullptr;
    };

    class ComputePassBuilder
    {
    public:
        ComputePassBuilder(RenderGraph* _pGraph, PassInfo* _pDesc);

        ComputePassBuilder& Enable(bool _bEnable = true);
        ComputePassBuilder& AsFinalPass(bool _bEnable = true);
        ComputePassBuilder& AsSideEffect(bool _bEnable = true);

        ComputePassBuilder& SetProgram(StringView _name);
        ComputePassBuilder& SetTexture(StringView _name, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        ComputePassBuilder& SetBuffer(StringView _name, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        ComputePassBuilder& SetConstantBuffer(StringView _name, Int32 _slot);
        ComputePassBuilder& SetSampler(Flags<eSamplerFlags> _state, Int32 _slot);
        ComputePassBuilder& SetSampler(Flags<eSamplerFlags> _state, RGBA _borderColor, Int32 _slot);

        RenderGraph::ComputePassBuilder& SetExecutor(const Delegate<void()>& _executor);

    private:
        RenderGraph* m_pGraph = nullptr;
        PassInfo*    m_pInfo  = nullptr;
    };

    RenderGraph() = default;
    ~RenderGraph();
    RenderGraph(const RenderGraph&)                = delete;
    RenderGraph& operator=(const RenderGraph&)     = delete;
    RenderGraph(RenderGraph&&) noexcept            = default;
    RenderGraph& operator=(RenderGraph&&) noexcept = default;

    ///////////////////////////////////
    ///       PASS MANAGEMENT       ///
    ///////////////////////////////////

    /// @brief 유틸리티 패스를 추가합니다.
    /// @param _name 패스 이름. 그래프 내에서 유일해야 합니다.
    /// @return 패스를 구성하는 데 사용할 UtilityPassBuilder 입니다.
    /// @note 유틸리티 패스는 실질적인 렌더링을 수행하지 않고, 주로 리소스 준비나 데이터 업로드 등의 작업에 사용됩니다.
    ///       유틸리티 패스는 자체적인 프레임버퍼를 가지지 않으며, Execute 과정에서 전달받은 프레임버퍼에 접근할 수 없습니다.
    ///       유틸리티 패스는 컬링되지 않고 반드시 실행됩니다.
    [[nodiscard]] UtilityPassBuilder AddUtilityPass(StringView _name);

    /// @brief 기존 유틸리티 패스를 수정합니다.
    /// @param _name 수정할 패스 이름.
    /// @return 패스를 수정하는 데 사용할 UtilityPassBuilder 입니다.
    [[nodiscard]] UtilityPassBuilder ModifyUtilityPass(StringView _name);

    /// @brief 렌더 패스를 추가합니다.
    /// @param _name 패스 이름. 그래프 내에서 유일해야 합니다.
    /// @return 패스를 구성하는 데 사용할 RenderPassBuilder 입니다.
    [[nodiscard]] RenderPassBuilder AddRenderPass(StringView _name);

    /// @brief 기존 렌더 패스를 수정합니다.
    /// @param _name 수정할 패스 이름.
    /// @return 패스를 수정하는 데 사용할 RenderPassBuilder 입니다.
    [[nodiscard]] RenderPassBuilder ModifyRenderPass(StringView _name);

    /// @brief 컴퓨트 패스를 추가합니다.
    /// @param _name 패스 이름. 그래프 내에서 유일해야 합니다.
    /// @return 패스를 구성하는 데 사용할 ComputePassBuilder 입니다.
    [[nodiscard]] ComputePassBuilder AddComputePass(StringView _name);

    /// @brief 기존 컴퓨트 패스를 수정합니다.
    /// @param _name 수정할 패스 이름.
    /// @return 패스를 수정하는 데 사용할 ComputePassBuilder 입니다.
    [[nodiscard]] ComputePassBuilder ModifyComputePass(StringView _name);

    /// @brief 패스를 제거합니다.
    /// @param _name 제거할 패스 이름.
    void RemovePass(StringView _name);

    /// @brief 이전 패스의 이름을 반환합니다.
    /// @param _step 몇 단계 이전인지를 지정합니다. 기본값은 1입니다.
    /// @return 이전 패스 이름.
    [[nodiscard]] StringView GetLastPass(Int32 _step = 1) const;

    /// @brief _passName 패스의 _slot 슬롯에 바인딩된 리소스 이름을 반환합니다.
    /// @param _passName 패스 이름.
    /// @param _slot 슬롯 번호. kDepthStencilAttachmentSlot 사용시 깊이/스텐실 버퍼를 참조합니다.
    /// @note 출력이 명시적이지 않은 패스의 경우 Assertion이 발생할 수 있습니다.
    /// @note Chain 형태의 패스를 구성할 때 유용한 함수입니다.
    [[nodiscard]] StringView GetPassAttachment(StringView _passName, Int32 _slot) const;

    /// @brief 그래프를 초기화합니다.
    /// @param _flags 초기화 범위 플래그. 기본값은 리소스와 패스 모두 초기화합니다.
    void Clear(Flags<eRenderGraphClearFlags> _flags = eRenderGraphClearFlags::All);

    /////////////////////////////////
    ///          RUNTIME          ///
    /////////////////////////////////

    /// @brief 그래프를 실행합니다.
    /// @param _fbh 최종 패스가 출력할 프레임버퍼 핸들.
    /// @param _vp 최종 패스에 적용할 뷰포트.
    void Execute(FrameBufferHandle _fbh, Viewport _vp);

    ///////////////////////////////////////////
    ///          TEXTURE MANAGEMENT         ///
    ///////////////////////////////////////////

    /// @brief 외부에서 생성된 텍스처를 그래프에 등록합니다.
    /// @param _name 텍스처 이름. 그래프 내에서 유일해야 합니다.
    /// @param _texh 등록할 텍스처 핸들.
    /// @param _bOwnership true이면 그래프가 소유권을 가지고 소멸 시 텍스처를 파괴합니다.
    void InsertTexture(StringView _name, TextureHandle _texh, bool _bOwnership = false);

    /// @brief 명시적 크기로 2D 텍스처를 생성하고 그래프에 등록합니다.
    /// @param _name 텍스처 이름. 그래프 내에서 유일해야 합니다.
    /// @param _width 텍스처 너비 (픽셀).
    /// @param _height 텍스처 높이 (픽셀).
    /// @param _format 텍스처 포맷.
    /// @param _bHasMips mipmap 레벨을 생성할지 여부.
    /// @param _numLayers 레이어 수. 기본값은 1입니다.
    /// @param _flags 텍스처 생성 플래그. 기본값은 None입니다.
    /// @param _initData 초기 텍셀 데이터. 비어있으면 초기화하지 않습니다.
    void EmplaceTexture2D(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, bool _bHasMips = false, Int32 _numLayers = 1, Flags<eTextureFlags> _flags = eTextureFlags::None, Span<const MemoryView> _initData = {});

    /// @brief 백버퍼 크기에 대한 비율로 2D 텍스처를 생성하고 그래프에 등록합니다.
    /// @param _name 텍스처 이름. 그래프 내에서 유일해야 합니다.
    /// @param _resizeScale 백버퍼 크기에 곱할 비율. 윈도우 리사이즈 시 자동으로 재생성됩니다.
    /// @param _format 텍스처 포맷.
    /// @param _bHasMips mipmap 레벨을 생성할지 여부.
    /// @param _numLayers 레이어 수. 기본값은 1입니다.
    /// @param _flags 텍스처 생성 플래그. 기본값은 None입니다.
    void EmplaceTexture2D(StringView _name, float _resizeScale, eTextureFormat _format, bool _bHasMips = false, Int32 _numLayers = 1, Flags<eTextureFlags> _flags = eTextureFlags::None);

    /// @brief 명시적 크기로 큐브 텍스처를 생성하고 그래프에 등록합니다.
    /// @param _name 텍스처 이름. 그래프 내에서 유일해야 합니다.
    /// @param _width 텍스처 너비 (픽셀).
    /// @param _height 텍스처 높이 (픽셀).
    /// @param _format 텍스처 포맷.
    /// @param _bHasMips mipmap 레벨을 생성할지 여부.
    /// @param _numCubes 큐브 수. 기본값은 1입니다.
    /// @param _flags 텍스처 생성 플래그. 기본값은 None입니다.
    void EmplaceTextureCube(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, bool _bHasMips, Int32 _numCubes = 1, Flags<eTextureFlags> _flags = eTextureFlags::None);

    /// @brief 백버퍼 크기에 대한 비율로 큐브 텍스처를 생성하고 그래프에 등록합니다.
    /// @param _name 텍스처 이름. 그래프 내에서 유일해야 합니다.
    /// @param _resizeScale 백버퍼 크기에 곱할 비율.
    /// @param _format 텍스처 포맷.
    /// @param _bHasMips
    /// @param _numCubes 큐브 수. 기본값은 1입니다.
    /// @param _flags 텍스처 생성 플래그. 기본값은 None입니다.
    void EmplaceTextureCube(StringView _name, float _resizeScale, eTextureFormat _format, bool _bHasMips, Int32 _numCubes = 1, Flags<eTextureFlags> _flags = eTextureFlags::None);

    /// @brief 3D 텍스처를 생성하고 그래프에 등록합니다.
    /// @param _name 텍스처 이름. 그래프 내에서 유일해야 합니다.
    /// @param _width 텍스처 너비 (픽셀).
    /// @param _height 텍스처 높이 (픽셀).
    /// @param _depth 텍스처 깊이 (슬라이스 수).
    /// @param _format 텍스처 포맷.
    /// @param _bHasMips
    /// @param _flags 텍스처 생성 플래그. 기본값은 None입니다.
    /// @param _initData 초기 텍셀 데이터. 비어있으면 초기화하지 않습니다.
    void EmplaceTexture3D(StringView _name, Int32 _width, Int32 _height, Int32 _depth, eTextureFormat _format, bool _bHasMips, Flags<eTextureFlags> _flags = eTextureFlags::None, Span<const MemoryView> _initData = {});

    /// @brief 텍스처를 그래프에서 제거합니다. 소유권이 있는 경우 텍스처를 파괴합니다.
    /// @param _name 제거할 텍스처 이름.
    void RemoveTexture(StringView _name);

    /// @brief 이름으로 텍스처 핸들을 가져옵니다.
    /// @param _name 텍스처 이름.
    /// @return 텍스처 핸들.
    [[nodiscard]] TextureHandle GetTexture(StringView _name) const;

    /// @brief 주어진 이름의 텍스처가 그래프에 등록되어 있는지 확인합니다.
    /// @param _name 확인할 텍스처 이름.
    /// @return 등록되어 있으면 true.
    [[nodiscard]] bool HasTexture(StringView _name) const;

    /// @brief 외부에서 생성된 상수 버퍼를 그래프에 등록합니다.
    /// @param _name 버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _cbh 등록할 상수 버퍼 핸들.
    /// @param _bOwnership true이면 그래프가 소유권을 가지고 소멸 시 버퍼를 파괴합니다.
    void InsertConstantBuffer(StringView _name, ConstantBufferHandle _cbh, bool _bOwnership);

    /// @brief 상수 버퍼를 생성하고 그래프에 등록합니다.
    /// @param _name 버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _byteWidth 버퍼 크기 (바이트).
    void EmplaceConstantBuffer(StringView _name, UInt32 _byteWidth);

    /// @brief 상수 버퍼를 그래프에서 제거합니다. 소유권이 있는 경우 버퍼를 파괴합니다.
    /// @param _name 제거할 버퍼 이름.
    void RemoveConstantBuffer(StringView _name);

    /// @brief 이름으로 상수 버퍼 핸들을 가져옵니다.
    /// @param _name 버퍼 이름.
    /// @return 상수 버퍼 핸들.
    [[nodiscard]] ConstantBufferHandle GetConstantBuffer(StringView _name) const;

    /// @brief 주어진 이름의 상수 버퍼가 그래프에 등록되어 있는지 확인합니다.
    /// @param _name 확인할 버퍼 이름.
    /// @return 등록되어 있으면 true.
    [[nodiscard]] bool HasConstantBuffer(StringView _name) const;

    /// @brief 외부에서 생성된 스토리지 버퍼를 그래프에 등록합니다.
    /// @param _name 버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _sbh 등록할 스토리지 버퍼 핸들.
    /// @param _bOwnership true이면 그래프가 소유권을 가지고 소멸 시 버퍼를 파괴합니다.
    void InsertStorageBuffer(StringView _name, StorageBufferHandle _sbh, bool _bOwnership);

    /// @brief Raw 스토리지 버퍼를 생성하고 그래프에 등록합니다.
    /// @param _name 버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _byteWidth 버퍼 크기 (바이트).
    /// @param _flags 버퍼 생성 플래그. 기본값은 None입니다.
    /// @param _initDataOrEmpty 초기 데이터. 비어있으면 초기화하지 않습니다.
    void EmplaceRawBuffer(StringView _name, Int32 _byteWidth, Flags<eStorageBufferFlags> _flags = eStorageBufferFlags::None, MemoryView _initDataOrEmpty = {});

    /// @brief 구조체 배열 스토리지 버퍼를 생성하고 그래프에 등록합니다.
    /// @param _name 버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _numElems 요소 수.
    /// @param _stride 요소 하나의 크기 (바이트).
    /// @param _type
    /// @param _flags 버퍼 생성 플래그. 기본값은 None입니다.
    /// @param _initDataOrEmpty 초기 데이터. 비어있으면 초기화하지 않습니다.
    void EmplaceStructuredBuffer(StringView _name, Int32 _numElems, Int32 _stride, eStructureBuffer _type = eStructureBuffer::Default, Flags<eStorageBufferFlags> _flags = eStorageBufferFlags::None, MemoryView _initDataOrEmpty = {});

    /// @brief 타입이 지정된 스토리지 버퍼를 생성하고 그래프에 등록합니다.
    /// @param _name 버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _numElems 요소 수.
    /// @param _dataType 요소의 데이터 타입.
    /// @param _flags 버퍼 생성 플래그. 기본값은 None입니다.
    /// @param _initDataOrEmpty 초기 데이터. 비어있으면 초기화하지 않습니다.
    void EmplaceTypedBuffer(StringView _name, Int32 _numElems, eTypedBufferFormat _dataType, Flags<eStorageBufferFlags> _flags = eStorageBufferFlags::None, MemoryView _initDataOrEmpty = {});

    /// @brief 인스턴드 드로우용 스토리지 버퍼를 생성하고 그래프에 등록합니다.
    /// @param _name 버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _numDraws 드로우 호출당 최대 인스턴스 수.
    void EmplaceIndirectBuffer(StringView _name, Int32 _numDraws);

    /// @brief 스토리지 버퍼를 그래프에서 제거합니다. 소유권이 있는 경우 버퍼를 파괴합니다.
    /// @param _name 제거할 버퍼 이름.
    void RemoveStorageBuffer(StringView _name);

    /// @brief 이름으로 스토리지 버퍼 핸들을 가져옵니다.
    /// @param _name 버퍼 이름.
    /// @return 스토리지 버퍼 핸들.
    [[nodiscard]] StorageBufferHandle GetStorageBuffer(StringView _name) const;

    /// @brief 주어진 이름의 스토리지 버퍼가 그래프에 등록되어 있는지 확인합니다.
    /// @param _name 확인할 버퍼 이름.
    /// @return 등록되어 있으면 true.
    [[nodiscard]] bool HasStorageBuffer(StringView _name) const;

    /// @brief 외부에서 생성된 프레임버퍼를 그래프에 등록합니다.
    /// @param _name 프레임버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _fbh 등록할 프레임버퍼 핸들.
    /// @param _bOwnership true이면 그래프가 소유권을 가지고 소멸 시 프레임버퍼를 파괴합니다.
    void InsertFrameBuffer(StringView _name, FrameBufferHandle _fbh, bool _bOwnership);

    /// @brief 여러 어태치먼트로 프레임버퍼를 생성하고 그래프에 등록합니다.
    /// @param _name 프레임버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _attachments 어태치먼트 목록.
    void EmplaceFrameBuffer(StringView _name, Span<const AttachmentRG> _attachments);

    /// @brief 단일 어태치먼트로 프레임버퍼를 생성하고 그래프에 등록합니다.
    /// @param _name 프레임버퍼 이름. 그래프 내에서 유일해야 합니다.
    /// @param _attachment 어태치먼트로 사용할 텍스처 이름.
    void EmplaceFrameBuffer(StringView _name, StringView _attachment);

    /// @brief 프레임버퍼를 그래프에서 제거합니다. 소유권이 있는 경우 프레임버퍼를 파괴합니다.
    /// @param _name 제거할 프레임버퍼 이름.
    void RemoveFrameBuffer(StringView _name);

    /// @brief 이름으로 프레임버퍼 핸들을 가져옵니다.
    /// @param _name 프레임버퍼 이름.
    /// @return 프레임버퍼 핸들.
    [[nodiscard]] FrameBufferHandle GetFrameBuffer(StringView _name) const;

    /// @brief 주어진 이름의 프레임버퍼가 그래프에 등록되어 있는지 확인합니다.
    /// @param _name 확인할 프레임버퍼 이름.
    /// @return 등록되어 있으면 true.
    [[nodiscard]] bool HasFrameBuffer(StringView _name) const;

    /// @brief 외부에서 생성된 셰이더를 그래프에 등록합니다.
    /// @param _name 셰이더 이름. 그래프 내에서 유일해야 합니다.
    /// @param _sh 등록할 셰이더 핸들.
    /// @param _bOwnership true이면 그래프가 소유권을 가지고 소멸 시 셰이더를 파괴합니다.
    void InsertShader(StringView _name, ShaderHandle _sh, bool _bOwnership);

    /// @brief 바이트코드로 셰이더를 생성하고 그래프에 등록합니다.
    /// @param _name 셰이더 이름. 그래프 내에서 유일해야 합니다.
    /// @param _bytecode 컴파일된 셰이더 바이트코드.
    void EmplaceShader(StringView _name, MemoryView _bytecode);

    /// @brief 셰이더를 그래프에서 제거합니다. 소유권이 있는 경우 셰이더를 파괴합니다.
    /// @param _name 제거할 셰이더 이름.
    void RemoveShader(StringView _name);

    /// @brief 이름으로 셰이더 핸들을 가져옵니다.
    /// @param _name 셰이더 이름.
    /// @return 셰이더 핸들.
    [[nodiscard]] ShaderHandle GetShader(StringView _name) const;

    /// @brief 주어진 이름의 셰이더가 그래프에 등록되어 있는지 확인합니다.
    /// @param _name 확인할 셰이더 이름.
    /// @return 등록되어 있으면 true.
    [[nodiscard]] bool HasShader(StringView _name) const;

    /// @brief 외부에서 생성된 프로그램을 그래프에 등록합니다.
    /// @param _name 프로그램 이름. 그래프 내에서 유일해야 합니다.
    /// @param _ph 등록할 프로그램 핸들.
    /// @param _bOwnership true이면 그래프가 소유권을 가지고 소멸 시 프로그램을 파괴합니다.
    void InsertProgram(StringView _name, ProgramHandle _ph, bool _bOwnership);

    /// @brief VS/PS 셰이더로 그래픽스 프로그램을 생성하고 그래프에 등록합니다.
    /// @param _name 프로그램 이름. 그래프 내에서 유일해야 합니다.
    /// @param _vsOrEmpty 버텍스 셰이더 이름. 비어있으면 해당 스테이지를 사용하지 않습니다.
    /// @param _psOrEmpty 픽셀 셰이더 이름. 비어있으면 해당 스테이지를 사용하지 않습니다.
    void EmplaceProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty);

    /// @brief VS/PS/GS/HS/DS 셰이더로 그래픽스 프로그램을 생성하고 그래프에 등록합니다.
    /// @param _name 프로그램 이름. 그래프 내에서 유일해야 합니다.
    /// @param _vsOrEmpty 버텍스 셰이더 이름.
    /// @param _psOrEmpty 픽셀 셰이더 이름.
    /// @param _gsOrEmpty 지오메트리 셰이더 이름.
    /// @param _hsOrEmpty 헐 셰이더 이름.
    /// @param _dsOrEmpty 도메인 셰이더 이름.
    void EmplaceProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty, StringView _gsOrEmpty, StringView _hsOrEmpty, StringView _dsOrEmpty);

    /// @brief 컴퓨트 셰이더로 컴퓨트 프로그램을 생성하고 그래프에 등록합니다.
    /// @param _name 프로그램 이름. 그래프 내에서 유일해야 합니다.
    /// @param _csName 컴퓨트 셰이더 이름.
    void EmplaceComputeProgram(StringView _name, StringView _csName);

    /// @brief 프로그램을 그래프에서 제거합니다. 소유권이 있는 경우 프로그램을 파괴합니다.
    /// @param _name 제거할 프로그램 이름.
    void RemoveProgram(StringView _name);

    /// @brief 이름으로 프로그램 핸들을 가져옵니다.
    /// @param _name 프로그램 이름.
    /// @return 프로그램 핸들.
    [[nodiscard]] ProgramHandle GetProgram(StringView _name) const;

    /// @brief 주어진 이름의 프로그램이 그래프에 등록되어 있는지 확인합니다.
    /// @param _name 확인할 프로그램 이름.
    /// @return 등록되어 있으면 true.
    [[nodiscard]] bool HasProgram(StringView _name) const;

private:
    void CompileIfNeed_(Int32 _width, Int32 _height);

    [[nodiscard]] PassInfo&       GetPassDesc_(StringView name);
    [[nodiscard]] const PassInfo& GetPassDesc_(StringView name) const;
    [[nodiscard]] bool            HasPass_(StringView name) const;

    Vector<PassInfo> m_infos         = {};
    Vector<Pass>     m_runtimePasses = {};

    StringHashMap<TextureResource>        m_textures       = {};
    StringHashMap<FrameBufferResource>    m_frameBuffers   = {};
    StringHashMap<ConstantBufferResource> m_cbuffers       = {};
    StringHashMap<StorageBufferResource>  m_storageBuffers = {};
    StringHashMap<ShaderResource>         m_shaders        = {};
    StringHashMap<ProgramResource>        m_programs       = {};

    bool  m_bNeedCompile = false;
    Int32 m_lastWidth    = -1;
    Int32 m_lastHeight   = -1;
};

}   // namespace Jam