#pragma once
#include "GraphicsBase.h"
#include "StringKeyContainers.h"

namespace Jam
{

enum class eRenderGraphClearFlags : UInt32
{
    Resource = 1 << 0,
    Pass     = 1 << 1,
    All      = Resource | Pass,
};

struct RenderGraphAttahment
{
    String name        = {};
    Int32  mipIndex    = 0;
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
            Murmur3Hasher32 hasher;
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

    struct PassDesc
    {
        String name = {};
        ePass  type = ePass::Utility;

        // graphics
        String        programName = {};
        ProgramHandle ph          = kInvalidHandle;

        // compute
        String        computeProgramName = {};
        ProgramHandle computePh          = kInvalidHandle;
        Int32         numThreadGroupsX   = 0;
        Int32         numThreadGroupsY   = 0;
        Int32         numThreadGroupsZ   = 0;

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
        bool bFinalPass  = false;
        bool bSideEffect = false;

        // Execute(fbh, vp) 호출 시 외부 출력을 이 패스에 주입할지 여부
        bool bUseExternalOutput = false;

        bool bEnable = true;
    };

    struct Pass
    {
        PassDesc* pDesc = nullptr;

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
    struct ResourceBase
    {
        String  name          = {};
        THandle handle        = kInvalidHandle;
        bool    bHasOwnership = false;
    };

    struct TextureResource : ResourceBase<TextureHandle>
    {
        bool                       bAutoResize = false;
        float                      resizeScale = 1.f;
        eTextureFormat             format      = eTextureFormat::Unknown;
        Int32                      arraySize   = 1;
        Flags<eTextureCreateFlags> flags       = eTextureCreateFlags::None;
    };

    struct FrameBufferResource : ResourceBase<FrameBufferHandle>
    {
        Array<String, kMaxNumAttachmentSlots> attachments      = {};
        Int32                                 numRenderTargets = 0;
        bool                                  bHasDepthStencil = false;
        bool                                  bAutoResize      = false;
    };

    using ConstantBufferResource = ResourceBase<ConstantBufferHandle>;
    using StorageBufferResource  = ResourceBase<StorageBufferHandle>;
    using ShaderResource         = ResourceBase<ShaderHandle>;
    using ProgramResource        = ResourceBase<ProgramHandle>;

public:
    class UtilityPassBuilder
    {
    public:
        UtilityPassBuilder(RenderGraph& _graph, PassDesc& _desc);

        UtilityPassBuilder& SetEnabled(bool _bEnable = true);
        UtilityPassBuilder& SetExecutor(const Delegate<void()>& _executor);

    private:
        RenderGraph& m_graph;
        PassDesc&    m_desc;
    };

    class RenderPassBuilder
    {
    public:
        RenderPassBuilder(RenderGraph& _graph, PassDesc& _desc);

        RenderPassBuilder& SetAsFinalPass(bool _bEnable = true);
        RenderPassBuilder& SetUseExternalOutput(bool _bEnable = true);
        RenderPassBuilder& SetEnabled(bool _bEnable = true);

        RenderPassBuilder& SetProgram(StringView _name);
        RenderPassBuilder& ReleaseProgram();

        RenderPassBuilder& SetFrameBuffer(StringView _name);
        RenderPassBuilder& ReleaseFrameBuffer();

        RenderPassBuilder& SetViewport(Viewport _vp);
        RenderPassBuilder& SetFrameBufferResolutionForViewport();

        RenderPassBuilder& SetScissorRect(Rect _rect);

        RenderPassBuilder& SetRenderFlags(Flags<eRenderFlags> _flags);
        RenderPassBuilder& SetStencilFlags(Flags<eStencilFlags> _fstencil, Flags<eStencilFlags> _bstencil, UInt8 _ref);

        RenderPassBuilder& SetBlendFlags(Flags<eBlendFlags> _flags, Int32 _slot);
        RenderPassBuilder& SetBlendFactor(RGBA _factor);

        RenderPassBuilder& SetRenderTargetClearCommand(RGBA _color, Int32 _slot);
        RenderPassBuilder& ReleaseRenderTargetCommand(Int32 _slot);
        RenderPassBuilder& SetDepthClearCommand(float _value);
        RenderPassBuilder& ReleaseDepthClearCommand();
        RenderPassBuilder& SetStencilClearCommand(UInt8 _value);
        RenderPassBuilder& ReleaseStencilClearCommand();

        RenderPassBuilder& SetPrevPassOutputAsResource(StringView _passName, Int32 _attachIndex, eShader _stage, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        RenderPassBuilder& SetTexture(StringView _name, eShader _stage, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        RenderPassBuilder& SetBuffer(StringView _name, eShader _stage, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);
        RenderPassBuilder& ReleaseResource(eShader _stage, Int32 _slot, eResourceAccess _access = eResourceAccess::Read);

        RenderPassBuilder& SetConstantBuffer(StringView _name, eShader _stage, Int32 _slot);
        RenderPassBuilder& ReleaseConstantBuffer(eShader _stage, Int32 _slot);

        RenderPassBuilder& SetSampler(Flags<eSamplerFlags> _state, eShader _stage, Int32 _slot);
        RenderPassBuilder& SetSampler(Flags<eSamplerFlags> _state, RGBA _borderColor, eShader _stage, Int32 _slot);
        RenderPassBuilder& ReleaseSamplerFlags(eShader _stage, Int32 _slot);

        RenderPassBuilder& SetExecutor(const Delegate<void()>& _executor);

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

    [[nodiscard]] UtilityPassBuilder AddUtilityPass(StringView _name);
    [[nodiscard]] UtilityPassBuilder ModifyUtilityPass(StringView _name);
    [[nodiscard]] RenderPassBuilder  AddGraphicsPass(StringView _name);
    [[nodiscard]] RenderPassBuilder  ModifyGrphicsPass(StringView _name);
    [[nodiscard]] ComputePassBuilder AddComputePass(StringView _name);
    [[nodiscard]] ComputePassBuilder ModifyComputePass(StringView _name);
    void                             RemovePass(StringView _name);

    void Clear(Flags<eRenderGraphClearFlags> _flags = eRenderGraphClearFlags::All);

    void Execute();
    void Execute(FrameBufferHandle _fbh, Viewport _vp);

    void                        AddTexture(StringView _name, TextureHandle _texh, bool _bOwnership = false);
    void                        AddTexture2D(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, Int32 _numLayers = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const MemoryView> _initData = {});
    void                        AddTexture2D(StringView _name, float _resizeScale, eTextureFormat _format, Int32 _numLayers = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None);
    void                        AddTexture3D(StringView _name, Int32 _width, Int32 _height, Int32 _depth, eTextureFormat _format, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const MemoryView> _initData = {});
    void                        AddTextureCube(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, Int32 _numCubes = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None);
    void                        RemoveTexture(StringView _name);
    [[nodiscard]] TextureHandle GetTexture(StringView _name) const;
    [[nodiscard]] TextureHandle FindTextureOrNull(StringView _name) const;

    void                               AddConstantBuffer(StringView _name, ConstantBufferHandle _cbh, bool _bOwnership);
    void                               AddConstantBuffer(StringView _name, UInt32 _byteWidth);
    void                               RemoveConstantBuffer(StringView _name);
    [[nodiscard]] ConstantBufferHandle GetConstantBuffer(StringView _name) const;
    [[nodiscard]] ConstantBufferHandle FindConstantBufferOrNull(StringView _name) const;

    void                              AddStorageBuffer(StringView _name, StorageBufferHandle _sbh, bool _bOwnership);
    void                              AddRawBuffer(StringView _name, Int32 _byteWidth, Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None, MemoryView _initDataOrEmpty = {});
    void                              AddStructuredBuffer(StringView _name, Int32 _numElements, Int32 _stride, bool _bAppendConsume = false, bool _bCounter = false, Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None, MemoryView _initDataOrEmpty = {});
    void                              AddTypedBuffer(StringView _name, Int32 _numElements, eStorageBufferData _dataType, Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None, MemoryView _initDataOrEmpty = {});
    void                              RemoveStorageBuffer(StringView _name);
    [[nodiscard]] StorageBufferHandle GetStorageBuffer(StringView _name) const;
    [[nodiscard]] StorageBufferHandle FindStorageBufferOrNull(StringView _name) const;

    void                            AddFrameBuffer(StringView _name, FrameBufferHandle _fbh, bool _bOwnership);
    void                            AddFrameBuffer(StringView _name, Span<const RenderGraphAttahment> _attachments);
    void                            AddFrameBuffer(StringView _name, StringView _attachment);
    void                            RemoveFrameBuffer(StringView _name);
    [[nodiscard]] FrameBufferHandle GetFrameBuffer(StringView _name) const;
    [[nodiscard]] FrameBufferHandle FindFrameBufferOrNull(StringView _name) const;

    void                       AddShader(StringView _name, ShaderHandle _sh, bool _bOwnership);
    void                       AddShader(StringView _name, MemoryView _bytecode);
    void                       RemoveShader(StringView _name);
    [[nodiscard]] ShaderHandle GetShader(StringView _name) const;
    [[nodiscard]] ShaderHandle FindShaderOrNull(StringView _name) const;

    void                        AddProgram(StringView _name, ProgramHandle _ph, bool _bOwnership);
    void                        AddProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty);
    void                        AddProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty, StringView _gsOrEmpty, StringView _hsOrEmpty, StringView _dsOrEmpty);
    void                        AddComputeProgram(StringView _name, StringView _csName);
    void                        RemoveProgram(StringView _name);
    [[nodiscard]] ProgramHandle GetProgram(StringView _name) const;
    [[nodiscard]] ProgramHandle FindProgramOrNull(StringView _name) const;

private:
    void CompileIfNeed_(Int32 _width, Int32 _height);
    void Execute_(FrameBufferHandle _fbh, Viewport _vp, bool _bUseExternalOutput);

    [[nodiscard]] PassDesc* FindPassDesc_(StringView name);
    [[nodiscard]] bool      HasPass_(StringView name) const;

    Vector<PassDesc> m_descs         = {};
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