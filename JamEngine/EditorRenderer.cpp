#include "pch.h"

#include "EditorRenderer.h"
#include "Graphics.h"
#include "MeshFactory.h"
#include "RenderView.h"
#include "ShaderCompiler.h"

// todo.. 수정..

namespace Jam
{

struct CBData
{
    Mtx4   viewProj;
    Mtx4   invViewProj;
    Mtx4   mtx;
    Vec3   eye;
    UInt32 flags;
    Vec4   color;
};

const char* const kGridShaderSource = R"(
static const float4 kAxisXCol = float4(253 / 255.f, 54 / 255.f, 83 / 255.f, 1.f);
static const float4 kAxisZCol = float4(48 / 255.f, 128 / 255.f, 219 / 255.f, 1.f);
static const float4 kMinorGridCol = float4(140.f / 255.f, 140.f / 255.f, 140.f / 255.f, 1.f);
static const float4 kMajorGridCol = kMinorGridCol * 0.9f;
static const float kGridCellSize = 1.f;
static const float kGridFadeStart = 80.f;
static const float kGridFadeEnd = 150.f;
static const float kMinorGridLineThickness = 1.f;
static const float kMajorGridLineThickness = 2.f;
static const float kAxisLineThickness = 2.f;
static const float kViewAngleFadeStart = 0.0f;
static const float kViewAngleFadeEnd = 0.20f;

#define DRAW_GRID_FLAG 1
#define DRAW_AXIS_FLAG 2

struct VS_INPUT
{
    float2 posNDC : POSITION;
};

struct PS_INPUT
{
    float4 posH   : SV_POSITION;
    float2 posNDC : TEXCOORD0;
};

cbuffer CBData : register(b0)
{
    float4x4 viewProj;
    float4x4 invViewProj;
    float4x4 mtx;
    float3   eye;
    uint     flags;
    float4   value;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.posH   = float4(input.posNDC, 0.0f, 1.0f);
    output.posNDC = input.posNDC;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    bool bDrawGrid = (flags & DRAW_GRID_FLAG) != 0;
    bool bDrawAxis = (flags & DRAW_AXIS_FLAG) != 0;

    // NDC → 월드 좌표 역투영
    float4 nearH = mul(float4(input.posNDC, 0.0f, 1.0f), invViewProj);
    float4 farH  = mul(float4(input.posNDC, 1.0f, 1.0f), invViewProj);
    float3 nearW = nearH.xyz / nearH.w;
    float3 farW  = farH.xyz  / farH.w;

    // Y=0 평면 교차
    float t = -nearW.y / (farW.y - nearW.y);
    clip(t - 0.0001f);

    float3 posW = nearW + t * (farW - nearW);

    // minor grid
    float2 gridCoord  = posW.xz / kGridCellSize;
    float2 fracCoord  = frac(gridCoord);
    float2 distToLine = min(fracCoord, 1.0 - fracCoord);
    float2 derivatives = max(fwidth(gridCoord), 0.001);
    float2 lineAA = distToLine / derivatives;
    float2 grid   = smoothstep(0.0, kMinorGridLineThickness, lineAA);
    float  minGrid = min(grid.x, grid.y);

    // major grid
    float2 majorGridCoord   = posW.xz / (kGridCellSize * 10.0);
    float2 majorFracCoord   = frac(majorGridCoord);
    float2 majorDistToLine  = min(majorFracCoord, 1.0 - majorFracCoord);
    float2 majorDerivatives = max(fwidth(majorGridCoord), 0.001);
    float2 majorLineAA      = majorDistToLine / majorDerivatives;
    float2 majorGrid        = smoothstep(0.0, kMajorGridLineThickness, majorLineAA);
    float  minMajorGrid     = min(majorGrid.x, majorGrid.y);

    // 거리 기반 페이드
    float dist         = distance(posW, eye);
    float fadeT        = 1.0 - smoothstep(kGridFadeStart, kGridFadeEnd, dist);
    float distanceFade = fadeT * fadeT * fadeT;

    // 시선 각도 기반 페이드
    float3 viewDir      = normalize(eye - posW);
    float3 gridNormal   = float3(0.0, 1.0, 0.0);
    float  viewDotNormal = abs(dot(viewDir, gridNormal));
    float  viewAngleFade = smoothstep(kViewAngleFadeStart, kViewAngleFadeEnd, viewDotNormal);

    // 최종 페이드
    float fadeFactor = distanceFade * viewAngleFade;

    // axis
    float2 axisDerivatives = max(float2(fwidth(posW.x), fwidth(posW.z)), 0.001);
    float  xAxis = smoothstep(0.0, kAxisLineThickness, abs(posW.z) / axisDerivatives.y);
    float  zAxis = smoothstep(0.0, kAxisLineThickness, abs(posW.x) / axisDerivatives.x);

    float xAxisAlpha    = 1.0 - xAxis;
    float zAxisAlpha    = 1.0 - zAxis;
    float majorGridAlpha = 1.0 - minMajorGrid;
    float minorGridAlpha = 1.0 - minGrid;

    float4 outColor = float4(0.0, 0.0, 0.0, 0.0);
    if (bDrawAxis && xAxisAlpha > 0.01)
    {
        outColor   = kAxisXCol;
        outColor.a = xAxisAlpha;
    }
    else if (bDrawAxis && zAxisAlpha > 0.01)
    {
        outColor   = kAxisZCol;
        outColor.a = zAxisAlpha;
    }
    else if (bDrawGrid && majorGridAlpha > 0.01)
    {
        outColor   = kMajorGridCol;
        outColor.a = majorGridAlpha;
    }
    else if (bDrawGrid && minorGridAlpha > 0.01)
    {
        outColor   = kMinorGridCol;
        outColor.a = minorGridAlpha;
    }

    outColor.a *= fadeFactor;
    return outColor;
}
)";

//////////////////////////////////
///      LINE SHADER           ///
//////////////////////////////////

const char* const kLineShaderSource = R"(
static const float kDepthBias = 0.0001f;

cbuffer CBData : register(b0)
{
    float4x4 viewProj;
    float4x4 invViewProj;
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
    output.posH.z -= kDepthBias * output.posH.w;
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

constexpr const char* kFilledShaderSource = R"(
cbuffer CB_EDITOR_RENDERER : register(b0)
{
    float4x4 viewProj;
    float4x4 invViewProj;
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

ProgramHandle CreateProgram_(
    const StringView _source)
{
    ShaderCompilerInfo sci = {};
    sci.stage              = eShader::VertexShader;
    sci.entryPoint         = "VSMain";
    sci.flags              = eShaderCompileFlags::ReleaseDefault;

    ShaderCompiler vsc = ShaderCompiler::CompileHLSLSource(_source, sci);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed");
    const ShaderHandle vsh = Graphics::GetInstance().CreateShader(vsc.GetBytecode());

    sci.stage      = eShader::PixelShader;
    sci.entryPoint = "PSMain";

    ShaderCompiler psc = ShaderCompiler::CompileHLSLSource(_source, sci);
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed");
    const ShaderHandle psh = Graphics::GetInstance().CreateShader(psc.GetBytecode());

    return Graphics::GetInstance().CreateProgram(vsh, psh, true);
}

}   // namespace Jam

namespace Jam
{

EditorRenderer::EditorRenderer()
{
    // constant buffer
    Graphics& gfx = Graphics::GetInstance();
    m_cbh         = gfx.CreateConstantBuffer(sizeof(CBData));
    gfx.SetConstantBufferName(m_cbh, "EditorRenderer_ConstantBuffer");

    m_gridPh   = CreateProgram_(kGridShaderSource);
    m_linePh   = CreateProgram_(kLineShaderSource);
    m_filledPh = CreateProgram_(kFilledShaderSource);

    // 라인 버퍼 생성
    {
        VertexLayout layout = {};
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3)
            .Add(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);

        m_lineVbh = gfx.CreateDynamicVertexBuffer(kLineLimit * 2, layout);
        m_lineIbh = gfx.CreateDynamicIndexBuffer(kLineLimit * 2);
    }

    // 그리드 쿼드
    {
        VertexLayout layout = {};
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float2);

        constexpr Vec2 kVerts[3] = {
            { -1.f,  1.f },
            {  3.f,  1.f },
            { -1.f, -3.f }
        };

        constexpr UInt16 kIdxes[3] = { 0, 1, 2 };

        m_screenTriVbh = gfx.CreateVertexBuffer(kVerts, layout);
        m_screenTriIbh = gfx.CreateIndexBuffer(kIdxes);
    }

    // 프리미티브 생성
    {
        VertexLayout attribLayout = {};
        attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);

        m_sphereMesh   = CreateSphereMesh(1.0f, 4, attribLayout);
        m_cylinderMesh = CreateCylinderMesh(1.0f, 1.0f, Vec3::kUp, 32, eCreateCylinderMesh::OriginAtCenter, attribLayout);
        m_cubeMesh     = CreateCubeMesh(Vec3::kOne, attribLayout);
    }

    // gfx state
    m_lineFlags   = eRenderFlags::LineAA | eRenderFlags::Primitive_LineList;
    m_gridFlags   = eRenderFlags::DepthFunc_Always;
    m_filledFlags = eRenderFlags::DepthFunc_LessEqual | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back;

    // blend (alpha blend)
    m_blendFlags = eBlendFlags::Enable
                 | eBlendFlags::SrcBlend_SrcAlpha | eBlendFlags::DstBlend_InvSrcAlpha | eBlendFlags::BlendOp_Add
                 | eBlendFlags::SrcBlendAlpha_One | eBlendFlags::DstBlendAlpha_Zero | eBlendFlags::BlendOpAlpha_Add
                 | eBlendFlags::WriteMask_All;
}

EditorRenderer::~EditorRenderer()
{
    JAM_DESTROY_VERTEX_BUFFER(m_screenTriVbh);
    JAM_DESTROY_INDEX_BUFFER(m_screenTriIbh);
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

    m_eye  = _view.eye;
    m_view = _view.view;
    m_proj = _view.proj;

    m_numLineVerts   = 0;
    m_numLineIndices = 0;

    m_drawGridFlags = eDrawGridFlags::None;
    m_bBeginRender  = true;

    Graphics& gfx = Graphics::GetInstance();
    gfx.SetFrameBuffer(_view.fbh);
    gfx.SetViewport(_view.vp);

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
    m_curColor = _color;
}

void EditorRenderer::DrawGrid(
    const Flags<eDrawGridFlags> _flags)
{
    m_drawGridFlags = _flags;
}

void EditorRenderer::DrawLine(const Vec3 _from, const Vec3 _to, const RGBA _color)
{
    if (m_numLineIndices + 2 >= kLineLimit * 2)
    {
        FlushLines_();
    }

    const UInt16 baseIdx = static_cast<UInt16>(m_numLineVerts);

    m_lineVertices[m_numLineVerts++]  = LineVertex { _from, _color };
    m_lineVertices[m_numLineVerts++]  = LineVertex { _to, _color };
    m_lineIndices[m_numLineIndices++] = baseIdx;
    m_lineIndices[m_numLineIndices++] = baseIdx + 1;
}

void EditorRenderer::DrawBoundingBox(const Aabb& _aabbInL, const Mtx4& _mtx, const RGBA _color)
{
    Obb obb = Obb { _aabbInL };
    obb     = obb.Transform(_mtx);

    const Corners corners = obb.GetCorners();

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
    const Frustum frustum = Frustum { _viewProjMtx };
    const Corners corners = frustum.CalcCorners();

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
    Vec3 u = Vec3::kUp;
    if (IsEqualApprox(_normal.Dot(u), 1.f))
    {
        u = Vec3::kRight;
    }

    const Vec3 tan   = _normal.Cross(u).Normalized();
    const Vec3 bitan = _normal.Cross(tan).Normalized();

    const float step = k2Pi / static_cast<float>(_segments);
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

    m_curColor = _color;

    const Mtx4 mtx = Mtx4::MakeSRT(Vec3 { _radius }, Quat::kIdentity, _center);
    UploadTransform_(mtx);

    Graphics&                 gfx   = Graphics::GetInstance();
    const Flags<eRenderFlags> flags = SetPrimitiveFlags(m_lineFlags, eRenderFlags::Primitive_TriangleList);
    gfx.SetRenderFlags(flags);
    gfx.SetProgram(m_filledPh);
    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    gfx.SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    m_sphereMesh->Submit();
}

void EditorRenderer::DrawFilledCube(const Vec3 _center, const Vec3 _size, const RGBA _color)
{
    FlushLines_();

    m_curColor = _color;

    const Mtx4 world = Mtx4::MakeScale(_size) * Mtx4::MakeTranslation(_center);
    UploadTransform_(world);

    Graphics&                 gfx   = Graphics::GetInstance();
    const Flags<eRenderFlags> flags = SetPrimitiveFlags(m_lineFlags, eRenderFlags::Primitive_TriangleList);
    gfx.SetRenderFlags(flags);
    gfx.SetProgram(m_filledPh);
    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    gfx.SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    m_cubeMesh->Submit();
}

void EditorRenderer::DrawFilledCylinder(const Vec3 _center, const Vec3 _dir, const float _height, const float _radius, const RGBA _color)
{
    FlushLines_();

    m_curColor       = _color;
    const Quat q     = Quat::FromLookTo(_dir, Vec3::kUp);
    const Mtx4 world = Mtx4::MakeScale(Vec3(_radius, _height, _radius)) * Mtx4::FromQuaternion(q) * Mtx4::MakeTranslation(_center);
    UploadTransform_(world);

    Graphics&                 gfx   = Graphics::GetInstance();
    const Flags<eRenderFlags> flags = SetPrimitiveFlags(m_lineFlags, eRenderFlags::Primitive_TriangleList);
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

    CBData data   = {};
    data.viewProj = (m_view * m_proj).Transpose();
    data.mtx      = Mtx4::kIdentity;
    data.eye      = m_eye;
    gfx.UpdateBuffer(m_cbh, AsMemoryView(data));

    gfx.UpdateBuffer(m_lineVbh, { m_lineVertices.data(), static_cast<Int32>(sizeof(LineVertex)) * m_numLineVerts });
    gfx.UpdateBuffer(m_lineIbh, { m_lineIndices.data(), static_cast<Int32>(sizeof(UInt16)) * m_numLineIndices });

    const Flags<eRenderFlags> flags = SetPrimitiveFlags(m_lineFlags, eRenderFlags::Primitive_LineList);
    gfx.SetRenderFlags(flags);
    gfx.SetProgram(m_linePh);
    gfx.SetVertexBuffer(m_lineVbh, m_numLineVerts, 0);
    gfx.SetIndexBuffer(m_lineIbh, 0, m_numLineIndices);
    gfx.Submit();

    m_numLineVerts   = 0;
    m_numLineIndices = 0;
}

void EditorRenderer::UploadTransform_(const Mtx4& _world) const
{
    Graphics& gfx = Graphics::GetInstance();

    CBData data   = {};
    data.viewProj = (m_view * m_proj).Transpose();
    data.mtx      = _world.Transpose();
    data.eye      = m_eye;
    data.color    = m_curColor.ToLinear();
    gfx.UpdateBuffer(m_cbh, AsMemoryView(data));
}

void EditorRenderer::DrawGrid_() const
{
    Graphics& gfx = Graphics::GetInstance();

    const Mtx4 vp = m_view * m_proj;

    CBData data      = {};
    data.viewProj    = vp.Transpose();
    data.invViewProj = vp.Inverse().Transpose();
    data.mtx         = Mtx4::kIdentity;
    data.eye         = m_eye;
    data.flags       = m_drawGridFlags.GetValue();
    gfx.UpdateBuffer(m_cbh, AsMemoryView(data));

    gfx.SetRenderFlags(m_gridFlags);
    gfx.SetBlendFlags(m_blendFlags, 0);
    gfx.SetProgram(m_gridPh);
    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    gfx.SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    gfx.SetVertexBuffer(m_screenTriVbh);
    gfx.SetIndexBuffer(m_screenTriIbh, 0, 6);
    gfx.Submit();
}

}   // namespace Jam