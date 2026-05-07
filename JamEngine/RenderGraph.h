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
        bool bFinalPass  = false;   // 최종 패스의 경우 자체적인 프레임버퍼를 가지지 않음. Execute 과정에서 전달받은 프레임버퍼에 렌더링함.
        bool bSideEffect = false;
        bool bEnable     = true;
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
        UtilityPassBuilder(RenderGraph* _pGraph, PassDesc* _pDesc);

        UtilityPassBuilder& Enable(bool _bEnable = true);
        UtilityPassBuilder& SetExecutor(const Delegate<void()>& _executor);

    private:
        RenderGraph* m_pGraph = nullptr;
        PassDesc*    m_pDesc  = nullptr;
    };

    class RenderPassBuilder
    {
    public:
        RenderPassBuilder(RenderGraph* _pGraph, PassDesc* _pDesc);

        RenderPassBuilder& Enable(bool _bEnable = true);
        RenderPassBuilder& AsFinalPass(bool _bEnable = true);

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
        RenderGraph* m_pGraph = nullptr;
        PassDesc*    m_pDesc  = nullptr;
    };

    class ComputePassBuilder
    {
    public:
        ComputePassBuilder(RenderGraph* _pGraph, PassDesc* _pDesc);

        ComputePassBuilder& Enable(bool _bEnable = true);
        ComputePassBuilder& AsFinalPass(bool _bEnable = true);

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
        RenderGraph* m_pGraph = nullptr;
        PassDesc*    m_pDesc  = nullptr;
    };

    RenderGraph() = default;
    ~RenderGraph();
    RenderGraph(const RenderGraph&)                = delete;
    RenderGraph& operator=(const RenderGraph&)     = delete;
    RenderGraph(RenderGraph&&) noexcept            = default;
    RenderGraph& operator=(RenderGraph&&) noexcept = default;

    // pass management
    [[nodiscard]] UtilityPassBuilder AddUtilityPass(StringView _name);
    [[nodiscard]] UtilityPassBuilder ModifyUtilityPass(StringView _name);
    [[nodiscard]] RenderPassBuilder  AddRenderPass(StringView _name);
    [[nodiscard]] RenderPassBuilder  ModifyGrphicsPass(StringView _name);
    [[nodiscard]] ComputePassBuilder AddComputePass(StringView _name);
    [[nodiscard]] ComputePassBuilder ModifyComputePass(StringView _name);
    void                             RemovePass(StringView _name);

    // pass desc access
    [[nodiscard]] String     GetPassNameAt(Int32 _index) const;
    [[nodiscard]] StringView GetPrevPassName(Int32 _step = 1) const;

    void Clear(Flags<eRenderGraphClearFlags> _flags = eRenderGraphClearFlags::All);
    void Execute(FrameBufferHandle _fbh, Viewport _vp);

    void                        InsertTexture(StringView _name, TextureHandle _texh, bool _bOwnership = false);
    void                        EmplaceTexture2D(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, Int32 _numLayers = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const MemoryView> _initData = {});
    void                        EmplaceTexture2D(StringView _name, float _resizeScale, eTextureFormat _format, Int32 _numLayers = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None);
    void                        EmplaceTexture3D(StringView _name, Int32 _width, Int32 _height, Int32 _depth, eTextureFormat _format, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const MemoryView> _initData = {});
    void                        EmplaceTextureCube(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, Int32 _numCubes = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None);
    void                        RemoveTexture(StringView _name);
    [[nodiscard]] TextureHandle GetTexture(StringView _name) const;
    [[nodiscard]] bool          HasTexture(StringView _name) const;

    void                               InsertConstantBuffer(StringView _name, ConstantBufferHandle _cbh, bool _bOwnership);
    void                               EmplaceConstantBuffer(StringView _name, UInt32 _byteWidth);
    void                               RemoveConstantBuffer(StringView _name);
    [[nodiscard]] ConstantBufferHandle GetConstantBuffer(StringView _name) const;
    [[nodiscard]] bool                 HasConstantBuffer(StringView _name) const;

    void                              InsertStorageBuffer(StringView _name, StorageBufferHandle _sbh, bool _bOwnership);
    void                              EmplaceRawBuffer(StringView _name, Int32 _byteWidth, Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None, MemoryView _initDataOrEmpty = {});
    void                              EmplaceStructuredBuffer(StringView _name, Int32 _numElements, Int32 _stride, bool _bAppendConsume = false, bool _bCounter = false, Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None, MemoryView _initDataOrEmpty = {});
    void                              EmplaceTypedBuffer(StringView _name, Int32 _numElements, eStorageBufferData _dataType, Flags<eStorageBufferCreateFlags> _flags = eStorageBufferCreateFlags::None, MemoryView _initDataOrEmpty = {});
    void                              RemoveStorageBuffer(StringView _name);
    [[nodiscard]] StorageBufferHandle GetStorageBuffer(StringView _name) const;
    [[nodiscard]] bool                HasStorageBuffer(StringView _name) const;

    void                            InsertFrameBuffer(StringView _name, FrameBufferHandle _fbh, bool _bOwnership);
    void                            EmplaceFrameBuffer(StringView _name, Span<const AttachmentRG> _attachments);
    void                            EmplaceFrameBuffer(StringView _name, StringView _attachment);
    void                            RemoveFrameBuffer(StringView _name);
    [[nodiscard]] FrameBufferHandle GetFrameBuffer(StringView _name) const;
    [[nodiscard]] bool              HasFrameBuffer(StringView _name) const;

    void                       InsertShader(StringView _name, ShaderHandle _sh, bool _bOwnership);
    void                       EmplaceShader(StringView _name, MemoryView _bytecode);
    void                       RemoveShader(StringView _name);
    [[nodiscard]] ShaderHandle GetShader(StringView _name) const;
    [[nodiscard]] bool         HasShader(StringView _name) const;

    void                        InsertProgram(StringView _name, ProgramHandle _ph, bool _bOwnership);
    void                        EmplaceProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty);
    void                        EmplaceProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty, StringView _gsOrEmpty, StringView _hsOrEmpty, StringView _dsOrEmpty);
    void                        EmplaceComputeProgram(StringView _name, StringView _csName);
    void                        RemoveProgram(StringView _name);
    [[nodiscard]] ProgramHandle GetProgram(StringView _name) const;
    [[nodiscard]] bool          HasProgram(StringView _name) const;

private:
    void CompileIfNeed_(Int32 _width, Int32 _height);
    void Execute_(FrameBufferHandle _fbh, Viewport _vp, bool _bUseExternalOutput);

    [[nodiscard]] PassDesc& GetPassDesc_(StringView name);
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