#include "pch.h"

#include "EditorRenderer.h"
#include "Graphics.h"
#include "MeshFactory.h"
#include "RenderView.h"
#include "ShaderCompiler.h"

namespace Jam
{

struct CB_EDITOR_RENDERER
{
    Mtx4   viewProj = Numbers<Mtx4>::k_identity;
    Mtx4   mtx      = Numbers<Mtx4>::k_identity;
    Vec3   eye      = Numbers<Vec3>::k_zero;
    UInt32 flags    = 0;
    Vec4   color    = Numbers<Vec4>::k_one;
};

//////////////////////////////////
///       GRID SHADER          ///
//////////////////////////////////

const char* const EditorRenderer::k_gridShaderSource = R"(
static const float4 k_axisXCol = float4(253 / 255.f, 54 / 255.f, 83 / 255.f, 1.f);
static const float4 k_axisZCol = float4(48 / 255.f, 128 / 255.f, 219 / 255.f, 1.f);
static const float4 k_minorGridCol = float4(140.f / 255.f, 140.f / 255.f, 140.f / 255.f, 1.f);
static const float4 k_majorGridCol = k_minorGridCol * 0.9f;
static const float k_gridCellSize = 1.f;
static const float k_gridFadeStart = 80.f;
static const float k_gridFadeEnd = 150.f;
static const float k_minorGridLineThickness = 1.f;
static const float k_majorGridLineThickness = 2.f;
static const float k_axisLineThickness = 2.f;
static const float k_viewAngleFadeStart = 0.0f;  
static const float k_viewAngleFadeEnd = 0.20f;   

#define DRAW_GRID_FLAG 1
#define DRAW_AXIS_FLAG 2

struct VS_INPUT
{
    float3 posL : POSITION;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
};

cbuffer CB_EDITOR_RENDERER : register(b0)
{
    float4x4 viewProj;
    float4x4 mtx;
    float3 eye;
    uint flags;
    float4 value;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.posW = input.posL;
    output.posH = mul(float4(output.posW, 1.0f), viewProj);
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    bool bDrawGrid = (flags & DRAW_GRID_FLAG) != 0;
    bool bDrawAxis = (flags & DRAW_AXIS_FLAG) != 0;
    
    float2 gridCoord = input.posW.xz / k_gridCellSize;
    float2 fracCoord = frac(gridCoord);
    float2 distToLine = min(fracCoord, 1.0 - fracCoord);
    
    float2 dx_grid = ddx(gridCoord);
    float2 dy_grid = ddy(gridCoord);
    float2 derivatives = float2(length(float2(dx_grid.x, dy_grid.x)), length(float2(dx_grid.y, dy_grid.y)));
    
    float2 lineAA = distToLine / derivatives;
    float2 grid = smoothstep(0.0, k_minorGridLineThickness, lineAA);
    float minGrid = min(grid.x, grid.y);
    
    float2 majorGridCoord = input.posW.xz / (k_gridCellSize * 10.0);
    float2 majorFracCoord = frac(majorGridCoord);
    float2 majorDistToLine = min(majorFracCoord, 1.0 - majorFracCoord);
    float2 majorDx = ddx(majorGridCoord);
    float2 majorDy = ddy(majorGridCoord);
    float2 majorDerivatives = float2(length(float2(majorDx.x, majorDy.x)), length(float2(majorDx.y, majorDy.y)));
    float2 majorLineAA = majorDistToLine / majorDerivatives;
    float2 majorGrid = smoothstep(0.0, k_majorGridLineThickness, majorLineAA);
    float minMajorGrid = min(majorGrid.x, majorGrid.y);
    
    // 거리 기반 페이드
    float dist = distance(input.posW, eye);
    float t = 1.0 - smoothstep(k_gridFadeStart, k_gridFadeEnd, dist);
    float distanceFade = t * t * t;
    
    // 시선 각도 기반 페이드
    float3 viewDir = normalize(eye - input.posW);
    float3 gridNormal = float3(0.0, 1.0, 0.0); 
    float viewDotNormal = abs(dot(viewDir, gridNormal));
    float viewAngleFade = smoothstep(k_viewAngleFadeStart, k_viewAngleFadeEnd, viewDotNormal);
    
    // 최종 페이드
    float fadeFactor = distanceFade * viewAngleFade;

    float xAxis = smoothstep(0.0, k_axisLineThickness, abs(input.posW.z) / derivatives.y);
    float zAxis = smoothstep(0.0, k_axisLineThickness, abs(input.posW.x) / derivatives.x);
    
    float xAxisAlpha = 1.0 - xAxis;
    float zAxisAlpha = 1.0 - zAxis;
    float majorGridAlpha = 1.0 - minMajorGrid;
    float minorGridAlpha = 1.0 - minGrid;
    
    float4 outColor = float4(0.0, 0.0, 0.0, 0.0);
    if (bDrawAxis && xAxisAlpha > 0.01)
    {
        outColor = k_axisXCol;
        outColor.a = xAxisAlpha;
    }
    else if (bDrawAxis && zAxisAlpha > 0.01)
    {
        outColor = k_axisZCol;
        outColor.a = zAxisAlpha;
    }
    else if (bDrawGrid && majorGridAlpha > 0.01)
    {
        outColor = k_majorGridCol;
        outColor.a = majorGridAlpha;
    }
    else if (bDrawGrid && minorGridAlpha > 0.01)
    {
        outColor = k_minorGridCol;
        outColor.a = minorGridAlpha;
    }
    
    outColor.a *= fadeFactor;
    return outColor;
}
)";

//////////////////////////////////
///      LINE SHADER          ///
//////////////////////////////////

const char* const EditorRenderer::k_lineShaderSource = R"(
static const float k_depthBias = 0.0001f;

cbuffer CB_EDITOR_RENDERER : register(b0)
{
    float4x4 viewProj;
    float4x4 mtx;
    float3   eye;
    float    pad;
    float4   value;
};

struct VS_INPUT
{
    float3 posL : POSITION;
    uint4 value : COLOR;
};

struct PS_INPUT
{
    float4 posH  : SV_POSITION;
    float4 value : COLOR;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    float4 posW = mul(float4(input.posL, 1.0f), mtx);
    output.posH = mul(posW, viewProj);
    output.posH.z -= k_depthBias * output.posH.w;
    output.value = float4(
        input.value.r / 255.0f,
        input.value.g / 255.0f,
        input.value.b / 255.0f,
        input.value.a / 255.0f
    );
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    return input.value;
}

)";

//////////////////////////////////
///    FILLED SHADER          ///
//////////////////////////////////

constexpr const char* k_editorRendererFilledShaderSource = R"(
cbuffer CB_EDITOR_RENDERER : register(b0)
{
    float4x4 viewProj;
    float4x4 mtx;
    float3 eye;
    float pad;
    float4 value;
};

struct VS_INPUT
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.posW = mul(float4(input.posL, 1.0f), mtx).xyz;
    output.normalW = mul(float4(input.normalL, 0.0f), mtx).xyz;
    output.posH = mul(float4(output.posW, 1.0f), viewProj);
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    // Simple lighting
    float3 normal = normalize(input.normalW);
    float3 lightDir = normalize(float3(1, 2, 1));
    float ndotl = max(0.0, dot(normal, lightDir));
    float3 lighting = value.rgb * (ndotl * 0.7 + 0.3);
    return float4(lighting, value.a);
}
)";

}   // namespace Jam

namespace Jam
{

EditorRenderer::EditorRenderer()
{
    LineVertex::Init();

    // constant buffer
    Graphics& gfx = Graphics::GetInstance();
    m_cbh         = gfx.CreateConstantBuffer(sizeof(CB_EDITOR_RENDERER));
    gfx.SetConstantBufferName(m_cbh, "EditorRenderer_ConstantBuffer");

    // Grid shaders
    {
        ShaderCompilerInfo vsDesc = {};
        vsDesc.stage              = eShader::VertexShader;
        vsDesc.entryPoint         = "VSMain";
        vsDesc.flags              = eShaderCompileFlags::ReleaseDefault;

        ShaderCompiler vsCompiler = ShaderCompiler::CompileHLSLSource(k_gridShaderSource, vsDesc);
        JAM_ASSERT(vsCompiler.IsSucceeded(), "Editor grid vertex shader compilation failed");
        const ShaderHandle gridVsh = gfx.CreateShader(vsCompiler.GetBytecode());

        ShaderCompilerInfo psDesc = {};
        psDesc.stage              = eShader::PixelShader;
        psDesc.entryPoint         = "PSMain";
        psDesc.flags              = eShaderCompileFlags::ReleaseDefault;

        ShaderCompiler psCompiler = ShaderCompiler::CompileHLSLSource(k_gridShaderSource, psDesc);
        JAM_ASSERT(psCompiler.IsSucceeded(), "Editor grid pixel shader compilation failed");
        const ShaderHandle gridPsh = gfx.CreateShader(vsCompiler.GetBytecode());

        m_gridPh = gfx.CreateProgram(gridVsh, gridPsh, true);
    }

    // Line shaders
    {
        ShaderCompilerInfo vsDesc = {};
        vsDesc.stage              = eShader::VertexShader;
        vsDesc.entryPoint         = "VSMain";
        vsDesc.flags              = eShaderCompileFlags::ReleaseDefault;

        ShaderCompiler vsCompiler = ShaderCompiler::CompileHLSLSource(k_lineShaderSource, vsDesc);
        JAM_ASSERT(vsCompiler.IsSucceeded(), "Editor line vertex shader compilation failed");
        const ShaderHandle lineVsh = gfx.CreateShader(vsCompiler.GetBytecode());

        ShaderCompilerInfo psDesc = {};
        psDesc.stage              = eShader::PixelShader;
        psDesc.entryPoint         = "PSMain";
        psDesc.flags              = eShaderCompileFlags::ReleaseDefault;

        ShaderCompiler psCompiler = ShaderCompiler::CompileHLSLSource(k_lineShaderSource, psDesc);
        JAM_ASSERT(psCompiler.IsSucceeded(), "Editor line pixel shader compilation failed");
        const ShaderHandle linePsh = gfx.CreateShader(psCompiler.GetBytecode());

        m_linePh = gfx.CreateProgram(lineVsh, linePsh, true);
    }

    // Filled shaders
    {
        ShaderCompilerInfo vsDesc = {};
        vsDesc.stage              = eShader::VertexShader;
        vsDesc.entryPoint         = "VSMain";
        vsDesc.flags              = eShaderCompileFlags::ReleaseDefault;

        ShaderCompiler vsCompiler = ShaderCompiler::CompileHLSLSource(k_editorRendererFilledShaderSource, vsDesc);
        JAM_ASSERT(vsCompiler.IsSucceeded(), "Editor filled vertex shader compilation failed");
        const ShaderHandle filledVsh = gfx.CreateShader(vsCompiler.GetBytecode());

        ShaderCompilerInfo psDesc = {};
        psDesc.stage              = eShader::PixelShader;
        psDesc.entryPoint         = "PSMain";
        psDesc.flags              = eShaderCompileFlags::ReleaseDefault;

        ShaderCompiler psCompiler = ShaderCompiler::CompileHLSLSource(k_editorRendererFilledShaderSource, psDesc);
        JAM_ASSERT(psCompiler.IsSucceeded(), "Editor filled pixel shader compilation failed");
        const ShaderHandle filledPsh = gfx.CreateShader(psCompiler.GetBytecode());

        m_filledPh = gfx.CreateProgram(filledVsh, filledPsh, true);
    }

    // 라인 버퍼 생성
    m_lineVbh = gfx.CreateDynamicVertexBuffer(k_lineLimit * 2, LineVertex::s_vl);
    m_lineIbh = gfx.CreateDynamicIndexBuffer(k_lineLimit * 2);

    // 그리드 쿼드 생성
    {
        constexpr float k_gridSize = 10000.0f;
        // todo: this is not infinite grid. grid cube need move with camera position

        struct GridVertex
        {
            Vec3 pos = Numbers<Vec3>::k_zero;
        };

        VertexLayout vl = {};
        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);

        constexpr GridVertex k_verts[4] = {
            { { -k_gridSize, 0.0f, -k_gridSize } },
            { { -k_gridSize, 0.0f, k_gridSize } },
            { { k_gridSize, 0.0f, k_gridSize } },
            { { k_gridSize, 0.0f, -k_gridSize } }
        };

        constexpr UInt16 k_idxes[6] = { 0, 1, 2, 0, 2, 3 };

        m_gridVbh = gfx.CreateVertexBuffer(k_verts, vl);
        m_gridIbh = gfx.CreateIndexBuffer(k_idxes);
    }

    // 프리미티브 생성
    {
        VertexLayout vl = {};
        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
        vl.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);

        m_sphereMesh   = CreateSphereMesh(1.0f, 4, vl);
        m_cylinderMesh = CreateCylinder(1.0f, 1.0f, 32, vl);
        m_cubeMesh     = CreateCubeMesh(Numbers<Vec3>::k_one, vl);
    }

    // gfx state
    m_lineRenderFlags   = eRenderFlags::LineAA | eRenderFlags::Primitive_LineList;
    m_gridRenderFlags   = eRenderFlags::DepthFunc_LessEqual | eRenderFlags::DepthWrite;
    m_filledRenderFlags = eRenderFlags::DepthFunc_LessEqual | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back;

    // blend (alpha blend)
    m_blendFlags = eBlendFlags::Enable
                 | eBlendFlags::SrcBlend_SrcAlpha | eBlendFlags::DstBlend_InvSrcAlpha | eBlendFlags::BlendOp_Add
                 | eBlendFlags::SrcBlendAlpha_One | eBlendFlags::DstBlendAlpha_Zero | eBlendFlags::BlendOpAlpha_Add
                 | eBlendFlags::WriteMask_All;
}

EditorRenderer::~EditorRenderer()
{
    JAM_DESTROY_VERTEX_BUFFER(m_gridVbh);
    JAM_DESTROY_INDEX_BUFFER(m_gridIbh);
    JAM_DESTROY_VERTEX_BUFFER(m_lineVbh);
    JAM_DESTROY_INDEX_BUFFER(m_lineIbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cbh);
    JAM_DESTROY_PROGRAM(m_gridPh);
    JAM_DESTROY_PROGRAM(m_linePh);
    JAM_DESTROY_PROGRAM(m_filledPh);
}

void EditorRenderer::Begin(const RenderView& _view)
{
    JAM_ASSERT(!m_bBeginRender, "EditorRenderer::Begin called without Exit");

    m_eye  = _view.eyePos;
    m_view = _view.view;
    m_proj = _view.proj;

    m_numLineVerts   = 0;
    m_numLineIndices = 0;

    m_drawGridFlags = eDrawGridFlags::None;
    m_bBeginRender  = true;

    Graphics& gfx = Graphics::GetInstance();
    gfx.SetFrameBuffer(_view.outputFbh);
    gfx.SetViewport(_view.outputVp);

    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    gfx.SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    gfx.SetBlendFlags(m_blendFlags, 0);
}

void EditorRenderer::End()
{
    JAM_ASSERT(m_bBeginRender, "EditorRenderer::Exit called without Begin");

    FlushLines_();

    if (m_drawGridFlags != eDrawGridFlags::None)
    {
        DrawGrid_();
    }

    m_bBeginRender = false;
}

void EditorRenderer::SetColor(const RGBA _color)
{
    m_currentColor = _color;
}

void EditorRenderer::DrawGrid(
    const Flags<eDrawGridFlags> _flags)
{
    m_drawGridFlags = _flags;
}

void EditorRenderer::DrawLine(const Vec3 _from, const Vec3 _to, const RGBA _color)
{
    if (m_numLineIndices + 2 >= k_lineLimit * 2)
    {
        FlushLines_();
    }

    const UInt16 baseIdx = NumericCast<UInt16>(m_numLineVerts);

    m_lineVertices[m_numLineVerts++]  = LineVertex { _from, _color };
    m_lineVertices[m_numLineVerts++]  = LineVertex { _to, _color };
    m_lineIndices[m_numLineIndices++] = baseIdx;
    m_lineIndices[m_numLineIndices++] = baseIdx + 1;
}

void EditorRenderer::DrawBoundingBox(const Aabb& _aabbInL, const Mtx4& _mtx, const RGBA _color)
{
    Obb obb = Obb::FromAabb(_aabbInL);
    obb.Transform(_mtx);

    const CornerPoints corners = obb.GetCornerPoints();

    DrawLine(corners[eRegion::LeftTopNear], corners[eRegion::RightTopNear], _color);
    DrawLine(corners[eRegion::RightTopNear], corners[eRegion::RightBottomNear], _color);
    DrawLine(corners[eRegion::RightBottomNear], corners[eRegion::LeftBottomNear], _color);
    DrawLine(corners[eRegion::LeftBottomNear], corners[eRegion::LeftTopNear], _color);

    // far plane
    DrawLine(corners[eRegion::LeftTopFar], corners[eRegion::RightTopFar], _color);
    DrawLine(corners[eRegion::RightTopFar], corners[eRegion::RightBottomFar], _color);
    DrawLine(corners[eRegion::RightBottomFar], corners[eRegion::LeftBottomFar], _color);
    DrawLine(corners[eRegion::LeftBottomFar], corners[eRegion::LeftTopFar], _color);

    // sides
    DrawLine(corners[eRegion::LeftTopNear], corners[eRegion::LeftTopFar], _color);
    DrawLine(corners[eRegion::RightTopNear], corners[eRegion::RightTopFar], _color);
    DrawLine(corners[eRegion::RightBottomNear], corners[eRegion::RightBottomFar], _color);
    DrawLine(corners[eRegion::LeftBottomNear], corners[eRegion::LeftBottomFar], _color);
}

void EditorRenderer::DrawFrustum(const Mtx4& _viewProjMtx, const RGBA _rgba)
{
    const Frustum      frustum = Frustum { _viewProjMtx };
    const CornerPoints corners = frustum.GetCornerPoints();

    // near plane
    DrawLine(corners[eRegion::LeftTopNear], corners[eRegion::RightTopNear], _rgba);
    DrawLine(corners[eRegion::RightTopNear], corners[eRegion::RightBottomNear], _rgba);
    DrawLine(corners[eRegion::RightBottomNear], corners[eRegion::LeftBottomNear], _rgba);
    DrawLine(corners[eRegion::LeftBottomNear], corners[eRegion::LeftTopNear], _rgba);

    // far plane
    DrawLine(corners[eRegion::LeftTopFar], corners[eRegion::RightTopFar], _rgba);
    DrawLine(corners[eRegion::RightTopFar], corners[eRegion::RightBottomFar], _rgba);
    DrawLine(corners[eRegion::RightBottomFar], corners[eRegion::LeftBottomFar], _rgba);
    DrawLine(corners[eRegion::LeftBottomFar], corners[eRegion::LeftTopFar], _rgba);

    // sides
    DrawLine(corners[eRegion::LeftTopNear], corners[eRegion::LeftTopFar], _rgba);
    DrawLine(corners[eRegion::RightTopNear], corners[eRegion::RightTopFar], _rgba);
    DrawLine(corners[eRegion::RightBottomNear], corners[eRegion::RightBottomFar], _rgba);
    DrawLine(corners[eRegion::LeftBottomNear], corners[eRegion::LeftBottomFar], _rgba);
}

void EditorRenderer::DrawCircle(const Vec3 _center, const Vec3 _normal, const float _radius, const RGBA _color, const Int32 _segments)
{
    Vec3 u = Numbers<Vec3>::k_up;
    if (IsEqualApprox(Dot(_normal, u), 1.f))
    {
        u = Numbers<Vec3>::k_right;
    }

    const Vec3 tan   = Normalized(Cross(_normal, u));
    const Vec3 bitan = Normalized(Cross(_normal, tan));

    const float step = Numbers<float>::k_2pi / static_cast<float>(_segments);
    for (Int32 i = 0; i < _segments; ++i)
    {
        const float a0 = step * static_cast<float>(i);
        const float a1 = step * static_cast<float>((i + 1) % _segments);
        const float s0 = Sin(a0);
        const float c0 = Cos(a0);
        const float s1 = Sin(a1);
        const float c1 = Cos(a1);
        const Vec3  p0 = _center + (tan * c0 + bitan * s0) * _radius;
        const Vec3  p1 = _center + (tan * c1 + bitan * s1) * _radius;
        DrawLine(p0, p1, _color);
    }
}

void EditorRenderer::FlushLines()
{
    FlushLines_();
}

void EditorRenderer::DrawFilledSphere(const Vec3 _center, const float _radius, const RGBA _color)
{
    FlushLines_();

    m_currentColor = _color;

    const Mtx4 world = Mtx4::CreateScale(_radius) * Mtx4::CreateTranslation(_center);
    UploadTransform_(world);

    Graphics&                 gfx   = Graphics::GetInstance();
    const Flags<eRenderFlags> flags = SetPrimitiveFlags(m_lineRenderFlags, eRenderFlags::Primitive_TriangleList);
    gfx.SetRenderFlags(flags);
    gfx.SetProgram(m_filledPh);
    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    gfx.SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    m_sphereMesh->Submit();
}

void EditorRenderer::DrawFilledCube(const Vec3 _center, const Vec3 _size, const RGBA _color)
{
    FlushLines_();

    m_currentColor = _color;

    const Mtx4 world = Mtx4::CreateScale(_size) * Mtx4::CreateTranslation(_center);
    UploadTransform_(world);

    Graphics&                 gfx   = Graphics::GetInstance();
    const Flags<eRenderFlags> flags = SetPrimitiveFlags(m_lineRenderFlags, eRenderFlags::Primitive_TriangleList);
    gfx.SetRenderFlags(flags);
    gfx.SetProgram(m_filledPh);
    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    gfx.SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    m_cubeMesh->Submit();
}

void EditorRenderer::DrawFilledCylinder(const Vec3 _center, const Vec3 _dir, const float _height, const float _radius, const RGBA _color)
{
    FlushLines_();

    m_currentColor   = _color;
    const Quat q     = Quat::FromToRotation(Numbers<Vec3>::k_up, _dir);
    const Mtx4 world = Mtx4::CreateScale(Vec3(_radius, _height, _radius)) * Mtx4::CreateFromQuaternion(q) * Mtx4::CreateTranslation(_center);
    UploadTransform_(world);

    Graphics&                 gfx   = Graphics::GetInstance();
    const Flags<eRenderFlags> flags = SetPrimitiveFlags(m_lineRenderFlags, eRenderFlags::Primitive_TriangleList);
    gfx.SetRenderFlags(flags);
    gfx.SetProgram(m_filledPh);
    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    gfx.SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    m_cylinderMesh->Submit();
}

void EditorRenderer::DrawFilledCapsule(const Vec3 _center, const Vec3 _dir, const float _height, const float _radius, const RGBA _color)
{
    const float cylinderHeight = Max(0.f, _height - 2.f * _radius);

    // 실린더
    DrawFilledCylinder(_center, _dir, cylinderHeight, _radius, _color);

    // 위쪽 반구
    const Vec3 topCenter = _center + _dir * cylinderHeight * 0.5f;
    DrawFilledSphere(topCenter, _radius, _color);

    // 아래쪽 반구
    const Vec3 bottomCenter = _center - _dir * cylinderHeight * 0.5f;
    DrawFilledSphere(bottomCenter, _radius, _color);
}

void EditorRenderer::FlushLines_()
{
    if (m_numLineIndices == 0)
    {
        return;
    }

    Graphics& gfx = Graphics::GetInstance();

    CB_EDITOR_RENDERER cb = {};
    cb.viewProj           = (m_view * m_proj).Transpose();
    cb.mtx                = Numbers<Mtx4>::k_identity;
    cb.eye                = m_eye;
    gfx.UpdateBuffer(m_cbh, AsMemoryView(cb));

    gfx.UpdateBuffer(m_lineVbh, { m_lineVertices.data(), NumericCast<Int32>(sizeof(LineVertex)) * m_numLineVerts });
    gfx.UpdateBuffer(m_lineIbh, { m_lineIndices.data(), NumericCast<Int32>(sizeof(UInt16)) * m_numLineIndices });

    const Flags<eRenderFlags> flags = SetPrimitiveFlags(m_lineRenderFlags, eRenderFlags::Primitive_LineList);
    gfx.SetRenderFlags(flags);
    gfx.SetProgram(m_linePh);
    gfx.SetVertexBuffer(m_lineVbh, 0, m_numLineVerts, 0);
    gfx.SetIndexBuffer(m_lineIbh, m_numLineIndices, 0);
    gfx.Submit();

    m_numLineVerts   = 0;
    m_numLineIndices = 0;
}

void EditorRenderer::UploadTransform_(const Mtx4& _world) const
{
    Graphics& gfx = Graphics::GetInstance();

    CB_EDITOR_RENDERER cb = {};
    cb.viewProj           = (m_view * m_proj).Transpose();
    cb.mtx                = _world.Transpose();
    cb.eye                = m_eye;
    cb.color              = m_currentColor.ToLinear();
    gfx.UpdateBuffer(m_cbh, AsMemoryView(cb));
}

void EditorRenderer::DrawGrid_() const
{
    Graphics& gfx = Graphics::GetInstance();

    CB_EDITOR_RENDERER cb = {};
    cb.viewProj           = (m_view * m_proj).Transpose();
    cb.mtx                = Numbers<Mtx4>::k_identity.Transpose();
    cb.eye                = m_eye;
    cb.flags              = m_drawGridFlags;
    gfx.UpdateBuffer(m_cbh, AsMemoryView(cb));

    gfx.SetRenderFlags(m_gridRenderFlags);
    gfx.SetBlendFlags(m_blendFlags, 0);
    gfx.SetProgram(m_gridPh);
    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    gfx.SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    gfx.SetVertexBuffer(m_gridVbh, 0);
    gfx.SetIndexBuffer(m_gridIbh, 6, 0);
    gfx.Submit();
}

}   // namespace Jam