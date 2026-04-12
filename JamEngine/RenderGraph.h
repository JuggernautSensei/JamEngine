#pragma once
#include "GraphicsBase.h"
#include "StringKeyContainers.h"

namespace Jam
{

enum class eRenderGraphResetFlags : UInt32
{
    Resource = 1 << 0,   // 렌더그래프가 관리하는 모든 리소스들을 초기화
    Pass     = 1 << 1,   // 렌더그래프의 Pass들을 초기화

    All = Resource | Pass,
};

class RenderGraph
{
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
            Murmur3Hasher32 hasher;
            hasher.Mix(AsMemoryView(_slot.stage));
            hasher.Mix(AsMemoryView(_slot.slot));
            return hasher.Finish();
        }
    };

    struct ResourceBindCommand
    {
        String          name        = {};
        UInt32          handleValue = k_nullHandleValue;
        eShaderResource type        = eShaderResource::Texture;
        Slot            slot        = {};
    };

    struct ConstantBufferBindCommand
    {
        String               name = {};
        ConstantBufferHandle cbh  = k_nullHandle;
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
        RGBA                 rgba  = RGBA::k_black;
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
        RGBA  color = RGBA::k_black;
        Int32 slot  = 0;
    };

    // Pass 설명자
    struct PassDesc
    {
        String name = {};

        // graphics pass
        String        programName = {};
        ProgramHandle ph          = k_nullHandle;   // lazy init in Compile()

        // compute pass
        String        computeProgramName = {};
        ProgramHandle computePh          = k_nullHandle;   // lazy init in Compile()
        Int32         numThreadGroupsX   = 0;
        Int32         numThreadGroupsY   = 0;
        Int32         numThreadGroupsZ   = 0;
        bool          bIsComputePass     = false;

        // framebuffer
        String            frameBufferName = {};
        FrameBufferHandle fbh             = k_nullHandle;   // lazy init in Compile()

        // viewport
        Viewport viewport                               = {};     // lazy init in Compile()
        bool     bUseViewportFromFrameBufferResoulution = true;   // 뷰포트를 프레임버퍼의 해상도로 설정함. 기본 설정이며 99%의 경우 true를 사용하면 됨.

        // Scissor
        Rect scissorRect = {};

        // state
        Flags<eRenderFlags>  renderFlags = {};
        Flags<eStencilFlags> fstencil    = {};
        Flags<eStencilFlags> bstencil    = {};
        UInt8                stencilRef  = 0;

        // blend
        Array<BlendFlagsBindCommand, k_maxNumRenderTargetViewSlots> blendFlagsBindCmds    = {};
        Int32                                                       numBlendFlagsBindCmds = 0;
        RGBA                                                        blendFactor           = RGBA::k_transparent;

        // clear commands
        Array<RenderTargetClearCommand, k_maxNumRenderTargetViewSlots> renderTargetClearCmds    = {};
        Int32                                                          numRenderTargetClearCmds = 0;
        bool                                                           bClearDepth              = false;
        float                                                          depthClearValue          = 1.f;
        bool                                                           bClearStencil            = false;
        UInt8                                                          stencilClearValue        = 0;

        // resource binding in this pass
        HashMap<Slot, ConstantBufferBindCommand, SlotHasher> cbufferBindCmds   = {};
        HashMap<Slot, SamplerFlagsBindCommand, SlotHasher>   samplerBindCmds   = {};
        HashMap<Slot, ResourceBindCommand, SlotHasher>       readBindCmds      = {};
        HashMap<Slot, ResourceBindCommand, SlotHasher>       readWriteBindCmds = {};

        // Pass 간의 의존성 분석과 최적화를 위해, 이 Pass가 읽거나 쓰는 리소스의 이름 목록을 별도로 유지함.
        // this will be filled in Compile()
        StringHashSet<> readResources      = {};
        StringHashSet<> readWriteResources = {};
        StringHashSet<> writeResources     = {};

        Delegate<void()> executor = nullptr;

        // 이 Pass가 최종 Pass인지를 나타냄.
        // 렌더 그래프 내에는 하나 이상의 최종 패스가 존재해야함.
        // 최종 패스에 영향을 못미치는 패스는 제거될 수 있음.
        bool bFinalPass = false;

        // Execute(FrameBufferHandle, Viewport) 형태로 실행될 때, 외부의 FrameBufferHandle과 Viewport를 주입받는데,
        // 이 때 주입받은 FrameBuffer와 Viewport를 사용할지 여부.
        // Execute() 형태로 실행될 때는 기존에 설정된 FrameBuffer와 Viewport를 사용함.
        bool bUseExternalOutput = false;

        bool bEnable = true;   // 이 Pass를 활성화할지 여부. false로 설정된 Pass는 컴파일과 실행 시 무시됨.
    };

    // 최종 실행될 Pass 데이터
    struct Pass
    {
        PassDesc* pDesc = nullptr;

        // 언바인드 목록
        Vector<Slot> readUnbindSlots      = {};
        Vector<Slot> readWriteUnbindSlots = {};
        bool         bUnbindFrameBuffer   = false;

        // 바인딩 목록
        Vector<ResourceBindCommand>       readBindCmds      = {};
        Vector<ResourceBindCommand>       readWriteBindCmds = {};
        Vector<ConstantBufferBindCommand> cbufferBindCmds   = {};
        Vector<SamplerFlagsBindCommand>   samplerBindCmds   = {};

        // 언바인드 -> touch -> 바인딩 -> 드로우콜
    };

    template<typename THandle>
    struct ResourceBase
    {
        String  name          = {};
        THandle handle        = k_nullHandle;
        bool    bHasOwnership = false;   // true: RenderGraph가 gpu 리소스의 수명을 관리함.
    };

    struct TextureResource : ResourceBase<TextureHandle>
    {
        bool                       bAutoResize = false;   // true: Compile시 백버퍼에 맞춰 자동으로 크기가 조정됨
        float                      resizeScale = 1.f;     // 자동 리사이즈 시 백버퍼 크기에 곱해지는 스케일. 예를 들어 0.5f로 설정하면 백버퍼의 절반 크기로 리사이즈됨.
        eTextureFormat             format      = eTextureFormat::Unknown;
        Int32                      arraySize   = 1;
        Flags<eTextureCreateFlags> flags       = eTextureCreateFlags::None;
    };

    struct FrameBufferResource : ResourceBase<FrameBufferHandle>
    {
        Array<String, k_maxNumAttachmentSlots> attachments    = {};
        Int32                                  numAttachments = 0;
        bool                                   bAutoResize    = false;   // true: Compile시 백버퍼에 맞춰 자동으로 크기가 조정됨
    };

    using ConstantBufferResource = ResourceBase<ConstantBufferHandle>;
    using StorageBufferResource  = ResourceBase<StorageBufferHandle>;
    using ShaderResource         = ResourceBase<ShaderHandle>;
    using ProgramResource        = ResourceBase<ProgramHandle>;

public:
    class PassBuilder
    {
    public:
        PassBuilder(RenderGraph& _graph, PassDesc& _desc);

        PassBuilder& SetAsFinalPass(bool _bEnable = true);
        PassBuilder& SetUseExternalOutput(bool _bEnable = true);
        PassBuilder& SetEnabled(bool _bEnable = true);

        PassBuilder& SetProgram(StringView _name);
        PassBuilder& ReleaseProgram();

        PassBuilder& SetFrameBuffer(StringView _name);
        PassBuilder& ReleaseFrameBuffer();

        PassBuilder& SetViewport(Viewport _vp);
        PassBuilder& SetViewportFromFrameBufferResolution();

        PassBuilder& SetScissorRect(Rect _rect);

        PassBuilder& SetRenderFlags(Flags<eRenderFlags> _flags);
        PassBuilder& SetStencilFlags(Flags<eStencilFlags> _fstencil, Flags<eStencilFlags> _bstencil, UInt8 _ref);

        PassBuilder& SetBlendFlags(Flags<eBlendFlags> _flags, Int32 _slot);
        PassBuilder& SetBlendFactor(RGBA _factor);

        PassBuilder& SetRenderTargetClearCommand(RGBA _color, Int32 _slot);
        PassBuilder& ReleaseRenderTargetCommand(Int32 _slot);
        PassBuilder& SetDepthClearCommand(float _value);
        PassBuilder& ReleaseDepthClearCommand();
        PassBuilder& SetStencilClearCommand(UInt8 _value);
        PassBuilder& ReleaseStencilClearCommand();

        PassBuilder& SetTexture(StringView _name, eShader _stage, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        PassBuilder& SetBuffer(StringView _name, eShader _stage, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        PassBuilder& ReleaseResource(eShader _stage, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);

        PassBuilder& SetConstantBuffer(StringView _name, eShader _stage, Int32 _slot);
        PassBuilder& ReleaseConstantBuffer(eShader _stage, Int32 _slot);

        PassBuilder& SetSampler(Flags<eSamplerFlags> _state, eShader _stage, Int32 _slot);
        PassBuilder& SetSampler(Flags<eSamplerFlags> _state, RGBA _borderColor, eShader _stage, Int32 _slot);
        PassBuilder& ReleaseSamplerFlags(eShader _stage, Int32 _slot);

        PassBuilder& SetExecutor(const Delegate<void()>& _executor);

    private:
        RenderGraph& m_graph;
        PassDesc&    m_desc;
    };

    class ComputePassBuilder
    {
    public:
        ComputePassBuilder(RenderGraph& _graph, PassDesc& _desc);

        ComputePassBuilder& SetAsFinalPass(bool _bEnable = true);
        ComputePassBuilder& SetEnable(bool _bEnable = true);

        ComputePassBuilder& SetComputeProgram(StringView _name);
        ComputePassBuilder& ReleaseComputeProgram();

        ComputePassBuilder& SetTexture(StringView _name, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        ComputePassBuilder& SetBuffer(StringView _name, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        ComputePassBuilder& ReleaseResource(Int32 _slot, eResourceAccess _access = eResourceAccess::Read);

        ComputePassBuilder& SetConstantBuffer(StringView _name, Int32 _slot);
        ComputePassBuilder& ReleaseConstantBuffer(Int32 _slot);

        ComputePassBuilder& SetSampler(Flags<eSamplerFlags> _state, Int32 _slot);
        ComputePassBuilder& SetSampler(Flags<eSamplerFlags> _state, RGBA _borderColor, Int32 _slot);
        ComputePassBuilder& ReleaseSamplerState(Int32 _slot);

        ComputePassBuilder& SetNumThreadGroups(Int32 _numThreadGroupsX, Int32 _numThreadGroupsY, Int32 _numThreadGroupsZ);

    private:
        RenderGraph& m_graph;
        PassDesc&    m_desc;
    };

    RenderGraph() = default;
    ~RenderGraph();
    RenderGraph(const RenderGraph&)                = delete;
    RenderGraph& operator=(const RenderGraph&)     = delete;
    RenderGraph(RenderGraph&&) noexcept            = default;
    RenderGraph& operator=(RenderGraph&&) noexcept = default;

    // pass builder
    [[nodiscard]] PassBuilder        AddPass(StringView _name);
    [[nodiscard]] PassBuilder        ModifyPass(StringView _name);
    [[nodiscard]] ComputePassBuilder AddComputePass(StringView _name);
    [[nodiscard]] ComputePassBuilder ModifyComputePass(StringView _name);
    void                             RemovePass(StringView _name);

    void Reset(Flags<eRenderGraphResetFlags> _flags = eRenderGraphResetFlags::All);

    // execute (compile if need)
    void Execute();
    void Execute(FrameBufferHandle _fbh, Viewport _vp);

    // texture resource management
    void                        AddTexture(StringView _name, TextureHandle _texh, bool _bOwnership = false);
    void                        AddTexture2D(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, Int32 _arraySize = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const MemoryView> _initData = {});
    void                        AddTexture2D(StringView _name, float _resizeScale, eTextureFormat _format, Int32 _arraySize = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None);
    void                        AddTexture3D(StringView _name, Int32 _width, Int32 _height, Int32 _depth, eTextureFormat _format, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const MemoryView> _initData = {});
    void                        AddTextureCube(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, Int32 _numCubes = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None);
    void                        RemoveTexture(StringView _name);
    [[nodiscard]] TextureHandle GetTexture(StringView _name) const;
    [[nodiscard]] TextureHandle FindTextureOrNull(StringView _name) const;

    // constant buffer resource management
    void                               AddConstantBuffer(StringView _name, ConstantBufferHandle _cbh, bool _bOwnership);
    void                               AddConstantBuffer(StringView _name, UInt32 _byteWidth);
    void                               RemoveConstantBuffer(StringView _name);
    [[nodiscard]] ConstantBufferHandle GetConstantBuffer(StringView _name) const;
    [[nodiscard]] ConstantBufferHandle FindConstantBufferOrNull(StringView _name) const;

    // storage buffer resource management
    void                              AddStorageBuffer(StringView _name, StorageBufferHandle _sbh, bool _bOwnership);
    void                              AddRawBuffer(StringView _name, Int32 _byteWidth, Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None, MemoryView _initDataOrEmpty = {});
    void                              AddStructuredBuffer(StringView _name, Int32 _numElements, Int32 _stride, bool _bAppendConsume = false, bool _bCounter = false, Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None, MemoryView _initDataOrEmpty = {});
    void                              AddTypedBuffer(StringView _name, Int32 _numElements, eStorageBufferData _dataType, Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None, MemoryView _initDataOrEmpty = {});
    void                              RemoveStorageBuffer(StringView _name);
    [[nodiscard]] StorageBufferHandle GetStorageBuffer(StringView _name) const;
    [[nodiscard]] StorageBufferHandle FindStorageBufferOrNull(StringView _name) const;

    // frame buffer resource management
    void                            AddFrameBuffer(StringView _name, FrameBufferHandle _fbh, bool _bOwnership);
    void                            AddFrameBuffer(StringView _name, Span<const StringView> _attachments);
    void                            RemoveFrameBuffer(StringView _name);
    [[nodiscard]] FrameBufferHandle GetFrameBuffer(StringView _name) const;
    [[nodiscard]] FrameBufferHandle FindFrameBufferOrNull(StringView _name) const;

    // shader resource management
    void                       AddShader(StringView _name, ShaderHandle _sh, bool _bOwnership);
    void                       AddShader(StringView _name, MemoryView _bytecode);
    void                       RemoveShader(StringView _name);
    [[nodiscard]] ShaderHandle GetShader(StringView _name) const;
    [[nodiscard]] ShaderHandle FindShaderOrNull(StringView _name) const;

    // program resource management
    void                        AddProgram(StringView _name, ProgramHandle _ph, bool _bOwnership);
    void                        AddProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty);
    void                        AddProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty, StringView _gsOrEmpty, StringView _hsOrEmpty, StringView _dsOrEmpty);
    void                        AddComputeProgram(StringView _name, StringView _csName);
    void                        RemoveProgram(StringView _name);
    [[nodiscard]] ProgramHandle GetProgram(StringView _name) const;
    [[nodiscard]] ProgramHandle FindProgramOrNull(StringView _name) const;

private:
    void CompileIfNeed_();
    void Execute_(FrameBufferHandle _fbh, Viewport _vp, bool _bUseExternalOutput);

    [[nodiscard]] PassDesc* FindPassDesc_(StringView name);   // 포인터 무효화에 주의
    [[nodiscard]] bool      HasPass_(StringView name) const;

    Vector<PassDesc> m_descs          = {};   // Pass 원본 데이터    (정렬 전)
    Vector<Pass>     m_compiledPasses = {};   // 위상 정렬 결과

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