#pragma once
#include "GraphicsBase.h"
#include "StringKeyContainers.h"

namespace Jam
{

// RenderGraph
// 렌더링 파이프라인 구성 요소들을 Pass 단위로 묶어서 표현하는 시스템
// 내부적으로 Pass 간의 자원 의존성을 분석하여, 최적의 실행 순서를 결정하고, 불필요한 Pass를 제거하는 등의 최적화 작업을 수행함.
//
// 주요기능:
// 1. 렌더링 파이프라인에서 사용되는 리소스를 관리.
//    문자열 및 디스크립터 기반으로 리소스의 생성 + 수명관리를 RenderGraph에게 위임할 수 있고,
//    참조형태로 리소스를 주입할 수 있음.
//
// 2. Pass 간의 의존성 분석.
//    Pass간의 read - write 리소스 관계에 따라 의존성 그래프를 구성하고
//    의존성이 안생기도록 Pass를 재구성함.
//    사용자는 기존까다로웠던 Pass간의 순서 처리, 리소스 바인딩 & 언바인딩을 전혀 신경쓰지 않아도 됨.
//    사용자는 오직 Pass를 작성하는데 집중하면 됨.
//
// 3. 그래프 최적화.
//    Pass간의 의존성 분석 결과를 바탕으로, 불필요한 Pass를 제거함.
//    더하여 Pass간의 리소스 바인딩을 최적화하여, 불필요한 바인딩과 언바인딩을 제거함.

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
            h.Mix(AsMemoryView(s.stage));
            h.Mix(AsMemoryView(s.index));
            return h.Finish();
        }
    };

    struct ReadBinding
    {
        String          name        = {};
        UInt32          handleValue = k_nullHandleValue;
        eShaderResource type        = eShaderResource::Texture;
        eShader         stage       = eShader::VertexShader;
        Int32           slot        = 0;
    };

    struct ReadWriteBinding
    {
        String          name        = {};
        UInt32          handleValue = k_nullHandleValue;
        eShaderResource type        = eShaderResource::Texture;
        eReadWriteStage stage       = eReadWriteStage::Pixel;
        Int32           slot        = 0;
    };

    struct ConstantBufferBinding
    {
        String               name  = {};
        ConstantBufferHandle cbh   = k_nullHandle;
        eShader              stage = eShader::VertexShader;
        Int32                slot  = 0;
    };

    struct SamplerBinding
    {
        Flags<eSamplerFlags> state = {};
        eShader              stage = eShader::VertexShader;
        Int32                slot  = 0;
    };

    struct BlendBinding
    {
        Flags<eBlendFlags> state = {};
        Int32              slot  = 0;
    };

    struct ClearRenderTargetBinding
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
        ProgramHandle ph          = k_nullHandle;   // Compile() 함수에서 지연결정됨.

        // compute
        ProgramHandle computePh = k_nullHandle;   // Compute Pass인 경우, 해당 Compute Shader를 실행함. Compile() 함수에서 지연결정됨

        // framebuffer
        String            frameBufferName = {};
        FrameBufferHandle fbh             = k_nullHandle;   // Compile() 함수에서 지연결정됨.

        // viewport
        Viewport viewport                            = {};
        bool     bUseFrameBufferResolutionAsViewport = true;   // 뷰포트를 프레임버퍼의 해상도로 설정함. 기본 설정이며 99%의 경우 true를 사용하면 됨.

        // misc
        bool bUseExternalOutput = false;
        // RenderGraph::Execute(FrameBufferHandle _fbh, Viewport _vp) 형태로 기대하는 프레임버퍼와 뷰포트를 외부에서 전달받을 수 있음
        // true로 설정된 경우, 해당 패스는 외부에서 주입된 출력으로 렌더링함.
        // 만약 RenderGraph::Execute() 호출 시 백버퍼에 렌더링함.

        // state
        Flags<eRenderFlags>  renderFlags = {};
        Flags<eStencilFlags> fstencil    = {};
        Flags<eStencilFlags> bstencil    = {};
        UInt8                stencilRef  = 0;

        // blend
        Array<BlendBinding, k_maxNumRenderTargetViewSlots> blendBinds    = {};
        Int32                                              numBlendBinds = 0;
        RGBA                                               blendFactor   = RGBA::k_transparent;

        // clear commands
        Array<ClearRenderTargetBinding, k_maxNumRenderTargetViewSlots> clearRenderTargetBinds = {};
        Int32                                                          numClearRenderTarget   = 0;
        float                                                          clearDepthValue        = 1.f;
        UInt8                                                          clearStencilValue      = 0;
        bool                                                           bClearDepth            = false;
        bool                                                           bClearStencil          = false;

        // resource binding. 리소스 바인딩은 Slot 단위로 관리됨
        HashMap<Slot, ReadBinding, SlotHasher>           textureBinds     = {};
        HashMap<Slot, ConstantBufferBinding, SlotHasher> cbufferBinds     = {};
        HashMap<Slot, SamplerBinding, SlotHasher>        samplerBinds     = {};
        Array<String, k_maxNumAttachmentSlots>           writeTextures    = {};   // 해당 패스의 framebuffer에서 사용되는 텍스처로, Compile() 함수에서 지연결정됨.
        Int32                                            numWriteTextures = 0;

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
        Vector<ReadBinding>           textures = {};
        Vector<ConstantBufferBinding> cbuffers = {};
        Vector<SamplerBinding>        samplers = {};

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

        PassBuilder& SetRenderFlags(Flags<eRenderFlags> _flags);
        PassBuilder& SetStencilFlags(Flags<eStencilFlags> _fstencil, Flags<eStencilFlags> _bstencil, UInt8 _ref);

        PassBuilder& SetBlendFlags(Flags<eBlendFlags> _flags, Int32 _slot);
        PassBuilder& SetBlendFactor(RGBA _factor);

        PassBuilder& SetClearColor(RGBA _color, Int32 _slot);
        PassBuilder& ReleaseClearColor(Int32 _slot);
        PassBuilder& SetClearDepth(float _value);
        PassBuilder& ReleaseClearDepth();
        PassBuilder& SetClearStencil(UInt8 _value);
        PassBuilder& ReleaseClearStencil();

        PassBuilder& SetTexture(StringView _name, eShader _stage, Int32 _slot);
        PassBuilder& SetConstantBuffer(StringView _name, eShader _stage, Int32 _slot);
        PassBuilder& SetSamplerState(Flags<eSamplerFlags> _state, eShader _stage, Int32 _slot);

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
    void                        AddTexture2D(StringView _name, Int32 _width, Int32 _height, eTextureFormat _format, Int32 _arraySize = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const MemoryView> _initData = {});
    void                        AddTexture2D(StringView _name, float _resizeScale, eTextureFormat _format, Int32 _arraySize = 1, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None);
    void                        AddTexture3D(StringView _name, Int32 _width, Int32 _height, Int32 _depth, eTextureFormat _format, Flags<eTextureCreateFlags> _flags = eTextureCreateFlags::None, Span<const MemoryView> _initData = {});
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
    void                       AddShader(StringView _name, MemoryView _bytecode);
    [[nodiscard]] ShaderHandle GetShader(StringView _name) const;
    [[nodiscard]] ShaderHandle FindShaderOrNull(StringView _name) const;

    void                        AddProgram(StringView _name, ProgramHandle _ph, bool _bOwnership);
    void                        AddProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty);
    void                        AddProgram(StringView _name, StringView _vsOrEmpty, StringView _psOrEmpty, StringView _gsOrEmpty, StringView _hsOrEmpty, StringView _dsOrEmpty);
    [[nodiscard]] ProgramHandle GetProgram(StringView _name) const;
    [[nodiscard]] ProgramHandle FindProgramOrNull(StringView _name) const;

private:
    void Reset_();

    [[nodiscard]] PassDesc* FindPassDesc_(StringView name);   // 포인터 무효화에 주의
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