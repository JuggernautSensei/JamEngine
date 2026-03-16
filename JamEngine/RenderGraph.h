#pragma once
#include "GraphicsBase.h"
#include "StringKeyContainers.h"

namespace Jam
{

// RenderGraph
// Pass 등록 순서와는 무관하게, 의존성을 분석하여 자동으로 실행 순서를 결정하는 렌더 그래프 시스템
// 내부적으로 위상 정렬을 사용

class RenderGraph
{
    struct Slot
    {
        [[nodiscard]] bool operator==(const Slot& other) const
        {
            return stage == other.stage && index == other.index;
        }

        eShader stage = eShader::VertexShader;
        Int32   index = 0;
    };

    struct SlotHasher
    {
        size_t operator()(const Slot& s) const noexcept
        {
            Murmur3Hasher64 h {};
            h.Mix(s.stage);
            h.Mix(s.index);
            return h.Finish();
        }
    };

    struct TextureBind
    {
        String        name  = {};
        TextureHandle texh  = k_nullHandle;
        eShader       stage = eShader::VertexShader;
        Int32         slot  = 0;
    };

    struct ConstantBufferBind
    {
        String               name  = {};
        ConstantBufferHandle cbh   = k_nullHandle;
        eShader              stage = eShader::VertexShader;
        Int32                slot  = 0;
    };

    struct SamplerBind
    {
        SamplerState state = {};
        eShader      stage = eShader::VertexShader;
        Int32        slot  = 0;
    };

    struct BlendStateBind
    {
        BlendState state = {};
        Int32      slot  = 0;
    };

    struct ClearColorBind
    {
        RGBA  color = RGBA::k_transparent;
        Int32 slot  = 0;
    };

    // Pass 원본 데이터
    struct PassDesc
    {
        String name = {};

        // program
        String        programName = {};
        ProgramHandle ph          = k_nullHandle;

        // framebuffer
        String            frameBufferName = {};
        FrameBufferHandle fbh             = k_nullHandle;

        // viewport
        Viewport viewport                            = {};
        bool     bUseFrameBufferResolutionAsViewport = true;   // 뷰포트를 프레임버퍼의 해상도로 설정함. 기본 설정임

        // misc
        bool bUseExternalOutput = false;
        // RenderGraph::Execute(FrameBufferHandle _fbh, Viewport _vp) 형태로 기대하는 프레임버퍼와 뷰포트를 외부에서 전달받을 수 있음
        // true로 설정된 경우, 해당 패스는 외부에서 주입된 출력으로 렌더링함.
        // 만약 RenderGraph::Execute() 호출 시 백버퍼에 렌더링함

        // state
        RenderState  renderState = {};
        StencilState fstencil    = {};
        StencilState bstencil    = {};
        UInt8        stencilRef  = 0;

        // blend
        Array<BlendStateBind, k_maxNumRenderTargetViewSlots> blendBinds         = {};
        Int32                                                numBlendStateBinds = 0;
        RGBA                                                 blendFactor        = RGBA::k_transparent;

        // clear commands
        Array<ClearColorBind, k_maxNumRenderTargetViewSlots> clearColorBinds            = {};
        Int32                                                numRenderTargetClearColors = 0;
        float                                                depthClearValue            = 1.f;
        UInt8                                                stencilClearValue          = 0;
        bool                                                 bClearDepth                = false;
        bool                                                 bClearStencil              = false;

        // resource binding
        HashMap<Slot, TextureBind, SlotHasher>        textureBindInfos = {};
        HashMap<Slot, ConstantBufferBind, SlotHasher> cbufferBindInfos = {};
        HashMap<Slot, SamplerBind, SlotHasher>        samplerBindInfos = {};
        Array<String, k_maxNumAttachmentSlots>        writeTextures    = {};
        Int32                                         numWriteTextures = 0;

        // 사이드 이펙트 여부
        // 위상정렬 특성상. 의존성이 아예없는 Pass가 존재할 수 있는데, 이런 Pass는 그래프 최적화 과정에서 제거됨.
        // 의존성이 없는 Pass를 Orphan Pass. 그리고 이런 Orphan Pass를 걸러내는 것을 컬링이라고 함.
        // 하지만, 떄떄로 의존성이 없는 Pass가 필요한 경우도 있음. bSideEffect를 true 일 때, orphan pass라도 제거되지 않고 남음.
        bool bSideEffect = false;

        // 실행 커맨드
        Delegate<void()> executor = nullptr;
    };

    // 최종 실행될 Pass 데이터
    struct Pass
    {
        const PassDesc* pDesc = nullptr;

        // 최적화된 바인딩
        Vector<TextureBind>        textureBinds = {};
        Vector<ConstantBufferBind> cbufferBinds = {};
        Vector<SamplerBind>        samplerBinds = {};

        // 언바인드 목록
        Vector<Slot> unbindTextures     = {};
        bool         bUnbindFrameBuffer = false;
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
        bool                       bAutoResize = false;   // true: Compile시 크기가 조정됨. false: Compile시 크기가 고정됨 (shadow texture 같은 경우 false로 설정)
        float                      resizeScale = 1.f;
        eTextureFormat             format      = eTextureFormat::Unknown;
        Int32                      arraySize   = 1;
        Flags<eTextureCreateFlags> flags       = eTextureCreateFlags::None;
    };

    struct FrameBufferResource : ResourceBase<FrameBufferHandle>
    {
        Array<String, k_maxNumAttachmentSlots> attachments    = {};
        Int32                                  numAttachments = 0;
    };

    using ConstantBufferResource = ResourceBase<ConstantBufferHandle>;
    using ShaderResource         = ResourceBase<ShaderHandle>;
    using ProgramResource        = ResourceBase<ProgramHandle>;

public:
    class PassBuilder
    {
    public:
        PassBuilder(RenderGraph& _graph, PassDesc& _decl)
            : m_graph(_graph)
            , m_desc(_decl)
        {
        }

        PassBuilder& SetSideEffect(bool _bEnable = true);

        PassBuilder& SetProgram(StringView _name);
        PassBuilder& ReleaseProgram();

        PassBuilder& SetFrameBuffer(StringView _name);
        PassBuilder& ReleaseFrameBuffer();

        PassBuilder& SetViewport(Viewport _vp);
        PassBuilder& SetFrameBufferResolutionAsViewport();

        PassBuilder& SetUseExternalOutput(bool _bUseExternalOutput = true);

        PassBuilder& SetRenderState(RenderState _state);
        PassBuilder& SetStencilState(StencilState _front, StencilState _back, UInt8 _ref);

        PassBuilder& SetBlendState(BlendState _state, Int32 _slot);
        PassBuilder& SetBlendFactor(RGBA _factor);

        PassBuilder& SetClearColor(RGBA _color, Int32 _slot);
        PassBuilder& ReleaseClearColor(Int32 _slot);
        PassBuilder& SetClearDepth(float _value);
        PassBuilder& ReleaseClearDepth();
        PassBuilder& SetClearStencil(UInt8 _value);
        PassBuilder& ReleaseClearStencil();

        PassBuilder& SetTexture(StringView _name, eShader _stage, Int32 _slot);
        PassBuilder& SetConstantBuffer(StringView _name, eShader _stage, Int32 _slot);
        PassBuilder& SetSamplerState(SamplerState _state, eShader _stage, Int32 _slot);

        PassBuilder& SetExecutor(const Delegate<void()>& _executor);

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
    [[nodiscard]] PassBuilder AddPass(StringView _name);
    [[nodiscard]] PassBuilder ModifyPass(StringView _name);
    void                      RemovePass(StringView _name);

    // compile & execute
    void               Compile(Int32 _width, Int32 _height);
    void               Execute();
    void               Execute(FrameBufferHandle _fbh, Viewport _vp);
    [[nodiscard]] bool NeedCompile() const;

    // resource management
    void                        AddTexture(StringView _name, TextureHandle _texh, bool _bOwnership);
    void                        AddTexture2D(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, Int32 _arraySize = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const CMemoryBlock> _initData = {});
    void                        AddTexture2D(StringView _name, float _resizeScale, eTextureFormat _format, Int32 _arraySize = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None);
    void                        AddTexture3D(StringView _name, Int32 _width, Int32 _height, Int32 _depth, eTextureFormat _format, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const CMemoryBlock> _initData = {});
    void                        AddTextureCube(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, Int32 _numCubes = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None);
    [[nodiscard]] TextureHandle GetTexture(StringView _name) const;
    [[nodiscard]] TextureHandle FindTextureOrNull(StringView _name) const;

    void                               AddConstantBuffer(StringView _name, ConstantBufferHandle _cbh, bool _bOwnership);
    void                               AddConstantBuffer(StringView _name, UInt32 _byteWidth);
    [[nodiscard]] ConstantBufferHandle GetConstantBuffer(StringView _name) const;
    [[nodiscard]] ConstantBufferHandle FindConstantBufferOrNull(StringView _name) const;

    void                            AddFrameBuffer(StringView _name, FrameBufferHandle _fbh, bool _bOwnership);
    void                            SetFrameBuffer(StringView _name, FrameBufferHandle _fbh, bool _bOwnership);
    void                            AddFrameBuffer(StringView _name, Span<const StringView> _attachments);
    void                            SetFrameBuffer(StringView _name, Span<const StringView> _attachments);
    [[nodiscard]] FrameBufferHandle GetFrameBuffer(StringView _name) const;
    [[nodiscard]] FrameBufferHandle FindFrameBufferOrNull(StringView _name) const;

    void                       AddShader(StringView _name, ShaderHandle _sh, bool _bOwnership);
    void                       AddShader(StringView _name, CMemoryBlock _bytecode);
    [[nodiscard]] ShaderHandle GetShader(StringView _name) const;
    [[nodiscard]] ShaderHandle FindShaderOrNull(StringView _name) const;

    void                        AddProgram(StringView _name, ProgramHandle _ph, bool _bOwnership);
    void                        AddProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty);
    void                        AddProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty, StringView _gsOrEmpty, StringView _hsOrEmpty, StringView _dsOrEmpty);
    [[nodiscard]] ProgramHandle GetProgram(StringView _name) const;
    [[nodiscard]] ProgramHandle FindProgramOrNull(StringView _name) const;

private:
    void Reset_();

    [[nodiscard]] PassDesc* FindPassDesc_(StringView name);
    [[nodiscard]] bool      HasPass_(StringView name) const;

    Vector<PassDesc> m_passDescs      = {};   // 정렬전 Pass 원본 데이터
    Vector<Pass>     m_compiledPasses = {};   // 위상 정렬 결과. 런타임용

    StringHashMap<TextureResource>        m_textures     = {};
    StringHashMap<ConstantBufferResource> m_cbuffers     = {};
    StringHashMap<FrameBufferResource>    m_frameBuffers = {};
    StringHashMap<ShaderResource>         m_shaders      = {};
    StringHashMap<ProgramResource>        m_programs     = {};

    bool  m_bNeedCompile = false;
    Int32 m_lastWidth    = -1;
    Int32 m_lastHeight   = -1;
};

}   // namespace Jam