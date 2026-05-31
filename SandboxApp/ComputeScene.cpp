// ReSharper disable CppMemberFunctionMayBeConst
#include "pch.h"
#include "ComputeScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RandomGenerator.h"
#include "Utils.h"
#include "../JamEngine/Algorithm.h"
#include "../JamEngine/EditorWidgets.h"
#include "../JamEngine/FileDialog.h"
#include "../JamEngine/Filter.h"
#include "../JamEngine/ImageExporter.h"
#include "../JamEngine/ImageImporter.h"
#include "../JamEngine/StaticString.h"

#define SCENE_PATH(_filepath) "Resources/ComputeScene/" _filepath

ComputeScene::ComputeScene()
{
}

void ComputeScene::OnUpdate(
    const float _deltaSec)
{
}

void ComputeScene::OnRender(
    const RenderView& _view)
{
    if (m_bDemoRunning)
    {
        m_graph.Execute(_view.fbh, _view.vp);
    }
}

void ComputeScene::OnImGuiRender()
{
    if (m_bDemoRunning)
    {
        ImGuiRenderOnDemoRun_();
    }
    else
    {
        ImGuiRenderOnDemoSelect_();
    }
}

void ComputeScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.FitAspectRatioAsBackBufferResolution();
}

void ComputeScene::ImGuiRenderOnDemoSelect_()
{
    const ImGuiViewport* pViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(pViewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(pViewport->WorkSize, ImGuiCond_Always);

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration;
    if (!ImGui::Begin("Select Demo", nullptr, kFlags))
    {
        ImGui::End();
        return;
    }

    // text
    // [padding]
    // listbox
    // [padding]
    // button

    // calc total item size
    constexpr Int32 kNumDemos      = static_cast<Int32>(CountOf(s_demos));
    constexpr float kListBoxWidth  = 320.0f;
    constexpr float kListBoxHeight = 120.f;

    const ImGuiStyle& style       = ImGui::GetStyle();
    const float       totalHeight = ImGui::GetFrameHeight() + ImGui::GetTextLineHeight() + style.ItemSpacing.y * 2.f + kListBoxHeight;

    const ImVec2 avail     = ImGui::GetContentRegionAvail();
    const ImVec2 totalSize = { kListBoxWidth, totalHeight };
    ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + (avail - totalSize) * 0.5f);

    ImGui::BeginGroup();
    {
        constexpr const char* kText    = "Select a demo, or double-click to run.";
        const ImVec2          textSize = ImGui::CalcTextSize(kText);
        ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2 { (kListBoxWidth - textSize.x) * 0.5f, 0.f });
        ImGui::TextDisabled(kText);

        if (ImGui::BeginListBox("##DemoList", { kListBoxWidth, kListBoxHeight }))
        {
            for (Int32 i = 0; i < kNumDemos; ++i)
            {
                const bool bSelected = (m_selectedDemo == i);
                if (ImGui::Selectable(s_demos[i].name, bSelected, ImGuiSelectableFlags_AllowDoubleClick))
                {
                    m_selectedDemo = i;

                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        RunSelectedDemo_();
                    }
                }

                if (bSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();
        }

        if (ImGui::Button("Run", { kListBoxWidth, 0.f }))
        {
            RunSelectedDemo_();
        }
    }
    ImGui::EndGroup();

    ImGui::End();
}

void ComputeScene::ImGuiRenderOnDemoRun_()
{
    const Demo& demo = s_demos[m_selectedDemo];

    StaticString<256> title;
    title.Format("ComputeScene {}", demo.name);
    if (!BeginSceneDemoWindow(title.GetPtr()))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Menu"))
    {
        m_bDemoRunning = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("Restart"))
    {
        RunSelectedDemo_();
    }

    if (demo.ImGuiFuncOrNull)
    {
        ImGui::Separator();
        (this->*demo.ImGuiFuncOrNull)();
    }

    ImGui::End();
}

void ComputeScene::RunSelectedDemo_()
{
    if (m_selectedDemo < 0 || m_selectedDemo >= CountOf(s_demos))
    {
        return;
    }

    // reset graphics
    Graphics& gfx = GetGraphics();
    gfx.Reset();

    // clear graph
    m_graph.Clear();

    // init demo
    const Demo& demo = s_demos[m_selectedDemo];
    (this->*demo.InitFunc)();

    m_bDemoRunning = true;
}

void ComputeScene::Demo_ImGui_OnlyComputeButton_()
{
    if (ImGui::Button("Compute"))
    {
        m_bBool0 = true;

        Graphics& gfx = GetGraphics();
        gfx.TriggerDebuggerCapture();
    }
}

void ComputeScene::InitDemo1_HelloCompute_()
{
    m_graph.EmplaceStructuredBuffer("B_OutBuffer", 64, sizeof(Int32), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    LoadProgram(m_graph, "P_HelloCompute", SCENE_PATH("HelloComputeShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_HelloCompute")
        .AsFinalPass()
        .SetProgram("P_HelloCompute")
        .SetBuffer("B_OutBuffer", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo1_Executor_>(this));

    m_bBool0 = false;
}

void ComputeScene::Demo1_Executor_()
{
    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();
        gfx.Dispatch(1, 1, 1);

        // copy
        const StorageBufferHandle src      = m_graph.GetStorageBuffer("B_OutBuffer");
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(64 * sizeof(Int32));
        gfx.CopyBuffer(readback, 0, src, 0);

        // read to cpu
        Vector<UInt32> buf(64, 0);
        gfx.ReadBuffer(readback, buf);
        JAM_LOG_INFO("========== Hello Compute Output ==========");
        for (SizeT i = 0; i < buf.size(); ++i)
        {
            JAM_LOG_INFO("buf[{}] = {}", i, buf[i]);
        }
        JAM_LOG_INFO("========== Done ==========");

        gfx.DestroyStorageBuffer(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::InitDemo2_AddConstant_()
{
    struct CBConstant
    {
        int   value;
        float padding[3];
    };

    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(CBConstant));
    m_graph.EmplaceStructuredBuffer("B_OutBuffer", 256, sizeof(Int32), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    LoadProgram(m_graph, "P_AddConstant", SCENE_PATH("AddConstantShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_AddConstant")
        .AsFinalPass()
        .SetProgram("P_AddConstant")
        .SetConstantBuffer("CB_Constant", 0)
        .SetBuffer("B_OutBuffer", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo2_Executor_>(this));

    m_bBool0 = false;
}

void ComputeScene::Demo2_Executor_()
{
    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();

        const ConstantBufferHandle cbh = m_graph.GetConstantBuffer("CB_Constant");
        gfx.UpdateBuffer(cbh, AsMemoryView(m_int0));

        gfx.Dispatch(4, 1, 1);

        // copy
        const StorageBufferHandle src      = m_graph.GetStorageBuffer("B_OutBuffer");
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(256 * sizeof(Int32));
        gfx.CopyBuffer(readback, 0, src, 0);

        // read to cpu
        Vector<UInt32> buf(256, 0);
        gfx.ReadBuffer(readback, buf);
        JAM_LOG_INFO("========== Add Constant Output ==========");
        JAM_LOG_INFO("Input Constant Value: {}", m_int0);
        for (SizeT i = 0; i < buf.size(); ++i)
        {
            JAM_LOG_INFO("buf[{}] = {}", i, buf[i]);
        }
        JAM_LOG_INFO("========== Done ==========");

        gfx.DestroyStorageBuffer(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::Demo2_ImGui_()
{
    ImGui::SliderInt("Constant Value", &m_int0, 0, 100);
    if (ImGui::Button("Compute"))
    {
        m_bBool0 = true;

        Graphics& gfx = GetGraphics();
        gfx.TriggerDebuggerCapture();
    }
}

void ComputeScene::InitDemo3_AddTwoBuffers_()
{
    m_graph.EmplaceStructuredBuffer("B_OutBuffer0", 1024, sizeof(float), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    m_graph.EmplaceStructuredBuffer("B_OutBuffer1", 1024, sizeof(float), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    m_graph.EmplaceStructuredBuffer("B_OutBuffer2", 1024, sizeof(float), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    LoadProgram(m_graph, "P_AddTwoBuffers", SCENE_PATH("AddTwoBuffersShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_AddTwoBuffers")
        .AsFinalPass()
        .SetProgram("P_AddTwoBuffers")
        .SetBuffer("B_OutBuffer0", 0, eResourceAccess::ReadWrite)
        .SetBuffer("B_OutBuffer1", 1, eResourceAccess::ReadWrite)
        .SetBuffer("B_OutBuffer2", 2, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo3_Executor_>(this));

    m_bBool0 = false;
}

void ComputeScene::Demo3_Executor_()
{
    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();
        gfx.Dispatch(16, 1, 1);

        // copy
        const StorageBufferHandle src0 = m_graph.GetStorageBuffer("B_OutBuffer0");
        const StorageBufferHandle src1 = m_graph.GetStorageBuffer("B_OutBuffer1");
        const StorageBufferHandle src2 = m_graph.GetStorageBuffer("B_OutBuffer2");

        Vector<float>             buf(1024, 0);
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(1024 * sizeof(float));

        JAM_LOG_INFO("========== Add Two Buffers Output ==========");

        JAM_LOG_INFO("Buffer 0:");
        gfx.CopyBuffer(readback, 0, src0, 0);
        gfx.ReadBuffer(readback, buf);
        for (SizeT i = 0; i < buf.size(); ++i)
        {
            JAM_LOG_INFO("buf[{}] = {}", i, buf[i]);
        }

        JAM_LOG_INFO("Buffer 1:");
        gfx.CopyBuffer(readback, 0, src1, 0);
        gfx.ReadBuffer(readback, buf);
        for (SizeT i = 0; i < buf.size(); ++i)
        {
            JAM_LOG_INFO("buf[{}] = {}", i, buf[i]);
        }

        JAM_LOG_INFO("Buffer 2 (Result of Adding Buffer 0 and Buffer 1):");
        gfx.CopyBuffer(readback, 0, src2, 0);
        gfx.ReadBuffer(readback, buf);
        for (SizeT i = 0; i < buf.size(); ++i)
        {
            JAM_LOG_INFO("buf[{}] = {}", i, buf[i]);
        }

        JAM_LOG_INFO("========== Done ==========");

        gfx.DestroyStorageBuffer(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::InitDemo4_Flatten_()
{
    struct CBConstant
    {
        UInt32 width;
        UInt32 height;
        float  padding[2];
    };

    struct Point2i
    {
        UInt32 x, y;
    };

    m_graph.EmplaceStructuredBuffer("B_OutBuffer", 256, sizeof(Point2i), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(CBConstant));
    LoadProgram(m_graph, "P_Flatten", SCENE_PATH("FlattenShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_Flatten")
        .AsFinalPass()
        .SetProgram("P_Flatten")
        .SetConstantBuffer("CB_Constant", 0)
        .SetBuffer("B_OutBuffer", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo4_Executor_>(this));

    m_bBool0 = false;
}

void ComputeScene::Demo4_Executor_()
{
    struct CBConstant
    {
        UInt32 width;
        UInt32 height;
        float  padding[2];
    };

    struct Point2i
    {
        UInt32 x, y;
    };

    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();

        CBConstant data;
        data.width                     = m_uint0;
        data.height                    = m_uint1;
        const ConstantBufferHandle cbh = m_graph.GetConstantBuffer("CB_Constant");
        gfx.UpdateBuffer(cbh, AsMemoryView(data));
        gfx.Dispatch(static_cast<Int32>(data.width + 7) / 8, static_cast<Int32>(data.height + 7) / 8, 1);

        // copy
        const StorageBufferHandle src      = m_graph.GetStorageBuffer("B_OutBuffer");
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(256 * sizeof(Point2i));
        gfx.CopyBuffer(readback, 0, src, 0);

        // read to cpu
        Vector<Point2i> buf(256);
        gfx.ReadBuffer(readback, buf);
        JAM_LOG_INFO("========== Flatten Output ==========");
        JAM_LOG_INFO("Width x Height: {} x {}", m_uint0, m_uint1);
        for (UInt32 i = 0; i < m_uint0 * m_uint1; ++i)
        {
            JAM_LOG_INFO("buf[{}] = ({}, {})", i, buf[i].x, buf[i].y);
        }
        JAM_LOG_INFO("========== Done ==========");
        gfx.DestroyStorageBuffer(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::Demo4_ImGui_()
{
    ImGui::SliderInt("Width", reinterpret_cast<int*>(&m_uint0), 1, 16);
    ImGui::SliderInt("Height", reinterpret_cast<int*>(&m_uint1), 1, 16);
    if (ImGui::Button("Compute"))
    {
        m_bBool0 = true;

        Graphics& gfx = GetGraphics();
        gfx.TriggerDebuggerCapture();
    }
}

void ComputeScene::InitDemo5_Gradiate_()
{
    struct CBConstant
    {
        UInt32 leftTop;
        UInt32 rightTop;
        UInt32 leftBottom;
        UInt32 rightBottom;
    };

    m_graph.EmplaceTexture2D("T_OutTexture", 256, 256, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::ShaderReadWrite);
    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(CBConstant));
    LoadProgram(m_graph, "P_Gradiate", SCENE_PATH("GradiateShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddComputePass("Pass_Gradiate")
        .SetProgram("P_Gradiate")
        .SetConstantBuffer("CB_Constant", 0)
        .SetTexture("T_OutTexture", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo5_Executor_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc blitDesc   = {};
    blitDesc.filter            = eImageFilter::Blit;
    blitDesc.input0Texture = "T_OutTexture";
    blitDesc.output            = eImageFilterOutput::Final;
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);

    m_rgba0 = RGBA::FromLinear(1.f, 0.f, 0.f, 1.f);
    m_rgba1 = RGBA::FromLinear(0.f, 1.f, 0.f, 1.f);
    m_rgba2 = RGBA::FromLinear(0.f, 0.f, 1.f, 1.f);
    m_rgba3 = RGBA::FromLinear(1.f, 1.f, 0.f, 1.f);
}

void ComputeScene::Demo5_Executor_() const
{
    struct CBConstant
    {
        UInt32 leftTop;
        UInt32 rightTop;
        UInt32 leftBottom;
        UInt32 rightBottom;
    };

    Graphics&                  gfx = GetGraphics();
    const ConstantBufferHandle cbh = m_graph.GetConstantBuffer("CB_Constant");

    CBConstant data;
    data.leftTop     = m_rgba0.ToPackedRGBA();
    data.rightTop    = m_rgba1.ToPackedRGBA();
    data.leftBottom  = m_rgba2.ToPackedRGBA();
    data.rightBottom = m_rgba3.ToPackedRGBA();
    gfx.UpdateBuffer(cbh, AsMemoryView(data));
    gfx.Dispatch(256 / 16, 256 / 16, 1);
}

void ComputeScene::Demo5_ImGui_()
{
    RGBAEdit3("Left Top Color", m_rgba0);
    RGBAEdit3("Right Top Color", m_rgba1);
    RGBAEdit3("Left Bottom Color", m_rgba2);
    RGBAEdit3("Right Bottom Color", m_rgba3);

    if (ImGui::Button("Capture"))
    {
        ExportTextureWithDialog_("T_OutTexture");
    }
}

void ComputeScene::InitDemo6_GrayScale_()
{
    struct CBConstant
    {
        UInt32 bGrayScale;
        float  padding[3];
    };

    Graphics& gfx = GetGraphics();

    Expect<ImageImporter> importer = ImageImporter::ImportFromFile(SCENE_PATH("Image0.jpg"));
    JAM_ASSERT(importer, "Failed to load image.");
    const TextureHandle texh = gfx.CreateTexture(importer->GetImageContainer());
    const TextureInfo   texi = gfx.GetTextureInfo(texh);
    m_int0                   = texi.width;
    m_int1                   = texi.height;

    m_graph.InsertTexture("T_InTexture", texh, true);
    m_graph.EmplaceTexture2D("T_OutTexture", texi.width, texi.height, texi.format, false, 1, eTextureFlags::ShaderReadWrite);
    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(CBConstant));
    LoadProgram(m_graph, "P_GrayScale", SCENE_PATH("GrayScaleShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddComputePass("Pass_GrayScale")
        .SetProgram("P_GrayScale")
        .SetTexture("T_InTexture", 0, eResourceAccess::Read)
        .SetTexture("T_OutTexture", 0, eResourceAccess::ReadWrite)
        .SetConstantBuffer("CB_Constant", 0)
        .SetExecutor(Bind<&ComputeScene::Demo6_Executor_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc blitDesc   = {};
    blitDesc.filter            = eImageFilter::Blit;
    blitDesc.input0Texture = "T_OutTexture";
    blitDesc.output            = eImageFilterOutput::Final;
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);

    m_bBool0 = true;
}

void ComputeScene::Demo6_Executor_()
{
    struct CBConstant
    {
        UInt32 bGrayScale;
        float  padding[3];
    };

    Graphics& gfx = GetGraphics();

    const ConstantBufferHandle cbh = m_graph.GetConstantBuffer("CB_Constant");
    CBConstant                 data;
    data.bGrayScale = m_bBool0;
    gfx.UpdateBuffer(cbh, AsMemoryView(data));

    gfx.Dispatch((m_int0 + 7) / 8, (m_int1 + 7) / 8, 1);
}

void ComputeScene::Demo6_ImGui_()
{
    ImGui::Checkbox("Apply GrayScale", &m_bBool0);
    if (ImGui::Button("Capture"))
    {
        ExportTextureWithDialog_("T_OutTexture");
    }
}

void ComputeScene::InitDemo7_ThreadGroupVisualizeShader_()
{
    m_graph.EmplaceTexture2D("T_OutTexture", 256, 256, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::ShaderReadWrite);
    LoadProgram(m_graph, "P_ThreadGroupVisualize", SCENE_PATH("ThreadGroupVisualizeShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddComputePass("Pass_ThreadGroupVisualize")
        .SetProgram("P_ThreadGroupVisualize")
        .SetTexture("T_OutTexture", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo7_Executor_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc blitDesc   = {};
    blitDesc.filter            = eImageFilter::Blit;
    blitDesc.input0Texture = "T_OutTexture";
    blitDesc.output            = eImageFilterOutput::Final;
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);
}

void ComputeScene::Demo7_Executor_() const
{
    Graphics& gfx = GetGraphics();
    gfx.Dispatch((256 + 7) / 8, (256 + 7) / 8, 1);
}

void ComputeScene::Demo_ImGui_OnlyCaptureButton_()
{
    if (ImGui::Button("Capture"))
    {
        ExportTextureWithDialog_("T_OutTexture");
    }
}

void ComputeScene::InitDemo8_BoundsCheck_()
{
    struct CBConstant
    {
        Int32 count;
        float padding[3];
    };

    m_graph.EmplaceStructuredBuffer("B_OutBuffer", 1024, sizeof(float), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(CBConstant));
    LoadProgram(m_graph, "P_BoundsCheck", SCENE_PATH("BoundsCheckShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_BoundsCheck")
        .AsFinalPass()
        .SetProgram("P_BoundsCheck")
        .SetBuffer("B_OutBuffer", 0, eResourceAccess::ReadWrite)
        .SetConstantBuffer("CB_Constant", 0)
        .SetExecutor(Bind<&ComputeScene::Demo8_Executor_>(this));

    m_bBool0 = false;
}

void ComputeScene::Demo8_Executor_()
{
    struct CBConstant
    {
        Int32 count;
        float padding[3];
    };

    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();

        CBConstant data;
        data.count = 1000;

        const ConstantBufferHandle cbh = m_graph.GetConstantBuffer("CB_Constant");
        gfx.UpdateBuffer(cbh, AsMemoryView(data));

        gfx.Dispatch(16, 1, 1);

        // copy
        const StorageBufferHandle src      = m_graph.GetStorageBuffer("B_OutBuffer");
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(1024 * sizeof(float));
        gfx.CopyBuffer(readback, 0, src, 0);

        // read to cpu
        Vector<float> buf(1024, 0);
        gfx.ReadBuffer(readback, buf);
        JAM_LOG_INFO("========== Bounds Check Output ==========");
        for (SizeT i = 0; i < buf.size(); ++i)
        {
            JAM_LOG_INFO("buf[{}] = {}", i, buf[i]);
        }
        JAM_LOG_INFO("========== Done ==========");

        gfx.DestroyStorageBuffer(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::InitDemo9_BlockSum_()
{
    Vector<Int32> init(1024, 0);
    std::ranges::generate(init, [n = 0]() mutable { return n++; });

    m_graph.EmplaceStructuredBuffer("B_InBuffer", 1024, sizeof(Int32), eStructureBuffer::Default, eStorageBufferFlags::ShaderRead, init);
    m_graph.EmplaceStructuredBuffer("B_OutBuffer", 16, sizeof(Int32), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    LoadProgram(m_graph, "P_BlockSum", SCENE_PATH("BlockSumShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_BlockSum")
        .AsFinalPass()
        .SetProgram("P_BlockSum")
        .SetBuffer("B_InBuffer", 0)
        .SetBuffer("B_OutBuffer", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo9_Executor_>(this));

    m_bBool0 = false;
}

void ComputeScene::Demo9_Executor_()
{
    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();
        gfx.Dispatch(16, 1, 1);

        // copy
        const StorageBufferHandle src      = m_graph.GetStorageBuffer("B_OutBuffer");
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(16 * sizeof(Int32));
        gfx.CopyBuffer(readback, 0, src, 0);

        // read to cpu
        Vector<Int32> buf(16, 0);
        gfx.ReadBuffer(readback, buf);
        JAM_LOG_INFO("========== Block Sum Output ==========");
        for (SizeT i = 0; i < buf.size(); ++i)
        {
            JAM_LOG_INFO("buf[{}] = {}", i, buf[i]);
        }
        JAM_LOG_INFO("========== Done ==========");

        gfx.DestroyStorageBuffer(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::InitDemo10_InterlockedAdd_()
{
    std::vector<Int32> init(4096, 0);
    std::ranges::generate(init, [n = 0]() mutable { return n++; });

    struct Params
    {
        Int32 threshold;
        float padding[3];
    };

    m_graph.EmplaceStructuredBuffer("B_InBuffer", 4096, sizeof(Int32), eStructureBuffer::Default, eStorageBufferFlags::ShaderRead, init);
    m_graph.EmplaceStructuredBuffer("B_OutBuffer", 1, sizeof(UInt32), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(Params));
    LoadProgram(m_graph, "P_InterlockedAdd", SCENE_PATH("InterlockedAddShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_InterlockedAdd")
        .AsFinalPass()
        .SetProgram("P_InterlockedAdd")
        .SetConstantBuffer("CB_Constant", 0)
        .SetBuffer("B_InBuffer", 0)
        .SetBuffer("B_OutBuffer", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo10_Executor_>(this));

    m_bBool0 = false;
}

void ComputeScene::Demo10_Executor_()
{
    struct Params
    {
        Int32 threshold;
        float padding[3];
    };

    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();

        const ConstantBufferHandle cbh = m_graph.GetConstantBuffer("CB_Constant");

        // update cbuffer
        Params data;
        data.threshold = m_int0;
        gfx.UpdateBuffer(cbh, AsMemoryView(data));

        // init src
        const StorageBufferHandle src  = m_graph.GetStorageBuffer("B_OutBuffer");
        UInt32                    zero = 0;
        gfx.UpdateBuffer(src, AsMemoryView(zero), 0, false);

        // dispatch
        gfx.Dispatch(64, 1, 1);

        // copy
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(1 * sizeof(UInt32));
        gfx.CopyBuffer(readback, 0, src, 0);

        // read to cpu
        UInt32 result = 0;
        gfx.ReadBuffer(readback, AsMutableMemoryView(result));
        JAM_LOG_INFO("========== Output ==========");
        JAM_LOG_INFO("Threshold = {}", m_int0);
        JAM_LOG_INFO("buf[0] = {}", result);
        JAM_LOG_INFO("========== Done ==========");

        gfx.DestroyStorageBuffer(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::Demo10_ImGui_()
{
    ImGui::SliderInt("Constant Value", &m_int0, -1, 4096);
    if (ImGui::Button("Compute"))
    {
        m_bBool0 = true;

        Graphics& gfx = GetGraphics();
        gfx.TriggerDebuggerCapture();
    }
}

void ComputeScene::InitDemo11_Negative_()
{
    struct CBConstant
    {
        UInt32 bNegative;
        float  padding[3];
    };

    Graphics& gfx = GetGraphics();

    Expect<ImageImporter> importer = ImageImporter::ImportFromFile(SCENE_PATH("Image1.jpg"));
    JAM_ASSERT(importer, "Failed to load image.");
    const TextureHandle texh = gfx.CreateTexture(importer->GetImageContainer());
    const TextureInfo   texi = gfx.GetTextureInfo(texh);
    m_int0                   = texi.width;
    m_int1                   = texi.height;

    m_graph.InsertTexture("T_InTexture", texh, true);
    m_graph.EmplaceTexture2D("T_OutTexture", texi.width, texi.height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::ShaderReadWrite);
    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(CBConstant));
    LoadProgram(m_graph, "P_Nagative", SCENE_PATH("NagativeShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddComputePass("Pass_Nagative")
        .SetProgram("P_Nagative")
        .SetTexture("T_InTexture", 0, eResourceAccess::Read)
        .SetTexture("T_OutTexture", 0, eResourceAccess::ReadWrite)
        .SetConstantBuffer("CB_Constant", 0)
        .SetExecutor(Bind<&ComputeScene::Demo11_Executor_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc blitDesc   = {};
    blitDesc.filter            = eImageFilter::Blit;
    blitDesc.input0Texture = "T_OutTexture";
    blitDesc.output            = eImageFilterOutput::Final;
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);

    m_bBool0 = true;
}

void ComputeScene::Demo11_Executor_()
{
    struct CBConstant
    {
        UInt32 bNegative;
        float  padding[3];
    };

    Graphics& gfx = GetGraphics();

    const ConstantBufferHandle cbh = m_graph.GetConstantBuffer("CB_Constant");
    CBConstant                 data;
    data.bNegative = m_bBool0;
    gfx.UpdateBuffer(cbh, AsMemoryView(data));

    gfx.Dispatch((m_int0 + 7) / 8, (m_int1 + 7) / 8, 1);
}

void ComputeScene::Demo11_ImGui_()
{
    ImGui::Checkbox("Apply Negative", &m_bBool0);
    if (ImGui::Button("Capture"))
    {
        ExportTextureWithDialog_("T_OutTexture");
    }
}

void ComputeScene::InitDemo12_Thresholding_()
{
    struct CBConstant
    {
        UInt32 bApply;
        float  threshold;
        float  padding[2];
    };

    Graphics& gfx = GetGraphics();

    Expect<ImageImporter> importer = ImageImporter::ImportFromFile(SCENE_PATH("Image2.jpg"));
    JAM_ASSERT(importer, "Failed to load image.");
    const TextureHandle texh = gfx.CreateTexture(importer->GetImageContainer());
    const TextureInfo   texi = gfx.GetTextureInfo(texh);
    m_int0                   = texi.width;
    m_int1                   = texi.height;

    m_graph.InsertTexture("T_InTexture", texh, true);
    m_graph.EmplaceTexture2D("T_OutTexture", texi.width, texi.height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::ShaderReadWrite);
    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(CBConstant));
    LoadProgram(m_graph, "P_Thresholding", SCENE_PATH("ThresholdingShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddComputePass("Pass_Thresholding")
        .SetProgram("P_Thresholding")
        .SetTexture("T_InTexture", 0, eResourceAccess::Read)
        .SetTexture("T_OutTexture", 0, eResourceAccess::ReadWrite)
        .SetConstantBuffer("CB_Constant", 0)
        .SetExecutor(Bind<&ComputeScene::Demo12_Executor_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc blitDesc   = {};
    blitDesc.filter            = eImageFilter::Blit;
    blitDesc.input0Texture = "T_OutTexture";
    blitDesc.output            = eImageFilterOutput::Final;
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);

    m_bBool0 = true;
    m_float0 = 0.5f;
}

void ComputeScene::Demo12_Executor_()
{
    struct CBConstant
    {
        UInt32 bApply;
        float  threshold;
        float  padding[2];
    };

    Graphics& gfx = GetGraphics();

    const ConstantBufferHandle cbh = m_graph.GetConstantBuffer("CB_Constant");
    CBConstant                 data;
    data.bApply    = m_bBool0;
    data.threshold = m_float0;
    gfx.UpdateBuffer(cbh, AsMemoryView(data));

    gfx.Dispatch((m_int0 + 15) / 16, (m_int1 + 15) / 16, 1);
}

void ComputeScene::Demo12_ImGui_()
{
    ImGui::Checkbox("Apply Thresholding", &m_bBool0);
    ImGui::DragFloat("Threshold", &m_float0, 0.001f, 0.f, 1.f);
    if (ImGui::Button("Capture"))
    {
        ExportTextureWithDialog_("T_OutTexture");
    }
}

void ComputeScene::InitDemo13_1DBoxBlur_()
{
    struct CBConstant
    {
        UInt32 bApply;
        UInt32 radius;
        float  padding[2];
    };

    Graphics& gfx = GetGraphics();

    Expect<ImageImporter> importer = ImageImporter::ImportFromFile(SCENE_PATH("Image3.jpg"));
    JAM_ASSERT(importer, "Failed to load image.");
    const TextureHandle texh = gfx.CreateTexture(importer->GetImageContainer());
    const TextureInfo   texi = gfx.GetTextureInfo(texh);
    m_int0                   = texi.width;
    m_int1                   = texi.height;

    m_graph.InsertTexture("T_InTexture", texh, true);
    m_graph.EmplaceTexture2D("T_OutTexture", texi.width, texi.height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::ShaderReadWrite);
    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(CBConstant));
    LoadProgram(m_graph, "P_1DBoxBlur", SCENE_PATH("1DBoxBlurShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddComputePass("Pass_1DBoxBlur")
        .SetProgram("P_1DBoxBlur")
        .SetTexture("T_InTexture", 0, eResourceAccess::Read)
        .SetTexture("T_OutTexture", 0, eResourceAccess::ReadWrite)
        .SetConstantBuffer("CB_Constant", 0)
        .SetExecutor(Bind<&ComputeScene::Demo13_Executor_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc blitDesc   = {};
    blitDesc.filter            = eImageFilter::Blit;
    blitDesc.input0Texture = "T_OutTexture";
    blitDesc.output            = eImageFilterOutput::Final;
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);

    m_bBool0 = true;
    m_int2   = 5;
}

void ComputeScene::Demo13_Executor_()
{
    struct CBConstant
    {
        UInt32 bApply;
        UInt32 radius;
        float  padding[2];
    };

    Graphics& gfx = GetGraphics();

    const ConstantBufferHandle cbh = m_graph.GetConstantBuffer("CB_Constant");

    CBConstant data;
    data.bApply = m_bBool0;
    data.radius = static_cast<UInt32>(m_int2);
    gfx.UpdateBuffer(cbh, AsMemoryView(data));

    gfx.Dispatch((m_int0 + 63) / 64, m_int1, 1);
}

void ComputeScene::Demo13_ImGui_()
{
    ImGui::Checkbox("Apply Blur", &m_bBool0);
    ImGui::DragInt("Radius", &m_int2, 1.f, 0, 512);
    if (ImGui::Button("Capture"))
    {
        ExportTextureWithDialog_("T_OutTexture");
    }
}

void ComputeScene::InitDemo14_FindMaximum_()
{
    Vector<UInt32> init(4096, 0);
    std::ranges::generate(init, [n = 0]() mutable { return n++; });

    RngMWC rng {};
    Shuffle(init, rng);

    m_graph.EmplaceStructuredBuffer("B_InBuffer", 4096, sizeof(UInt32), eStructureBuffer::Default, eStorageBufferFlags::ShaderRead, init);
    m_graph.EmplaceStructuredBuffer("B_OutBuffer", 1, sizeof(UInt32), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    LoadProgram(m_graph, "P_FindMax", SCENE_PATH("FindMaxShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_FindMax")
        .AsFinalPass()
        .SetProgram("P_FindMax")
        .SetBuffer("B_InBuffer", 0)
        .SetBuffer("B_OutBuffer", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo14_Executor_>(this));

    m_bBool0 = false;
    m_bBool1 = false;
}

void ComputeScene::Demo14_Executor_()
{
    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();

        // init src
        const StorageBufferHandle src  = m_graph.GetStorageBuffer("B_OutBuffer");
        UInt32                    zero = 0;
        gfx.UpdateBuffer(src, AsMemoryView(zero), 0, false);

        // dispatch
        gfx.Dispatch(64, 1, 1);

        // copy
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(1 * sizeof(UInt32));
        gfx.CopyBuffer(readback, 0, src, 0);

        // read to cpu
        UInt32 result = 0;
        gfx.ReadBuffer(readback, AsMutableMemoryView(result));
        JAM_LOG_INFO("========== Output ==========");
        if (m_bBool1)
        {
            const StorageBufferHandle readback2 = gfx.CreateReadbackBuffer(4096 * sizeof(UInt32));
            gfx.CopyBuffer(readback2, 0, m_graph.GetStorageBuffer("B_InBuffer"), 0);

            Vector<UInt32> mem(4096, 0);
            gfx.ReadBuffer(readback2, mem);
            for (SizeT i = 0; i < 4096; ++i)
            {
                JAM_LOG_INFO("InBuffer[{}] = {}", i, mem[i]);
            }
            JAM_LOG_INFO("======================");

            gfx.DestroyStorageBuffer(readback2);
        }
        JAM_LOG_INFO("Maximum = {}", result);
        JAM_LOG_INFO("========== Done ==========");

        gfx.DestroyStorageBuffer(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::InitDemo15_ParalleReduction_()
{
    Vector<float> init(65536, 0.f);
    std::ranges::generate(init, [n = 0.f]() mutable { return n++; });

    m_graph.EmplaceStructuredBuffer("B_InBuffer", 65536, sizeof(float), eStructureBuffer::Default, eStorageBufferFlags::ShaderRead, init);
    m_graph.EmplaceStructuredBuffer("B_GroupSums", 256, sizeof(float), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);
    m_graph.EmplaceStructuredBuffer("B_FinalSum", 1, sizeof(float), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);

    LoadProgram(m_graph, "P_ParalleReductionShader1", SCENE_PATH("ParalleReductionShader1.hlsl"), eShaderFlags::Compute);
    LoadProgram(m_graph, "P_ParalleReductionShader2", SCENE_PATH("ParalleReductionShader2.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_ParalleReduction1")
        .SetProgram("P_ParalleReductionShader1")
        .SetBuffer("B_InBuffer", 0)
        .SetBuffer("B_GroupSums", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo15_Executor1_>(this));

    m_graph.AddComputePass("Pass_ParalleReduction2")
        .AsFinalPass()
        .SetProgram("P_ParalleReductionShader2")
        .SetBuffer("B_GroupSums", 0, eResourceAccess::ReadWrite)
        .SetBuffer("B_FinalSum", 1, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo15_Executor2_>(this));

    m_bBool0 = false;
    m_bBool1 = false;
}

void ComputeScene::Demo15_Executor1_()
{
    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();

        // init src
        const StorageBufferHandle src = m_graph.GetStorageBuffer("B_GroupSums");
        Vector<float>             init(256, 0);
        gfx.UpdateBuffer(src, init, 0, false);

        // dispatch
        gfx.Dispatch(256, 1, 1);

        // copy
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(256 * sizeof(float));
        gfx.CopyBuffer(readback, 0, src, 0);

        // read to cpu
        Vector<float> readData(256, 0.f);
        gfx.ReadBuffer(readback, readData);
        JAM_LOG_INFO("========== Output ==========");
        for (Int32 i = 0; i < 256; ++i)
        {
            JAM_LOG_INFO("GroupSum[{}] = {}", i, readData[i]);
        }
        JAM_LOG_INFO("========== Done ==========");

        gfx.DestroyStorageBuffer(readback);
    }
}

void ComputeScene::Demo15_Executor2_()
{
    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();

        // init src
        const StorageBufferHandle src  = m_graph.GetStorageBuffer("B_FinalSum");
        float                     zero = 0.f;
        gfx.UpdateBuffer(src, AsMemoryView(zero), 0, false);

        // dispatch
        gfx.Dispatch(1, 1, 1);

        // copy
        const StorageBufferHandle readback = gfx.CreateReadbackBuffer(sizeof(float));
        gfx.CopyBuffer(readback, 0, src, 0);

        // read to cpu
        float result;
        gfx.ReadBuffer(readback, AsMutableMemoryView(result));
        JAM_LOG_INFO("========== Output ==========");
        JAM_LOG_INFO("FinalSum = {}", result);
        JAM_LOG_INFO("========== Done ==========");

        gfx.DestroyStorageBuffer(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::InitDemo16_GaussianBlur_()
{
    struct CBConstant
    {
        UInt32 bApply;
        UInt32 padding[3];
        float  weights[8];
    };

    Graphics& gfx = GetGraphics();

    Expect<ImageImporter> importer = ImageImporter::ImportFromFile(SCENE_PATH("Image4.jpg"));
    JAM_ASSERT(importer, "Failed to load image.");
    const TextureHandle texh = gfx.CreateTexture(importer->GetImageContainer());
    const TextureInfo   texi = gfx.GetTextureInfo(texh);
    m_int0                   = texi.width;
    m_int1                   = texi.height;

    m_graph.InsertTexture("T_InTexture", texh, true);
    m_graph.EmplaceTexture2D("T_TmpTexture", texi.width, texi.height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::ShaderReadWrite);
    m_graph.EmplaceTexture2D("T_OutTexture", texi.width, texi.height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::ShaderReadWrite);
    m_graph.EmplaceConstantBuffer("CB_Constant", sizeof(CBConstant));

    ShaderMacro macro[] = {
        { "VERTICAL", "" }
    };
    LoadProgram(m_graph, "P_GaussianBlurH", SCENE_PATH("GaussianBlurShader.hlsl"), eShaderFlags::Compute);
    LoadProgram(m_graph, "P_GaussianBlurV", SCENE_PATH("GaussianBlurShader.hlsl"), eShaderFlags::Compute, macro);   // VERTICAL 매크로 필요

    m_graph.AddComputePass("Pass_GaussianBlurH")
        .SetProgram("P_GaussianBlurH")
        .SetTexture("T_InTexture", 0, eResourceAccess::Read)
        .SetTexture("T_TmpTexture", 0, eResourceAccess::ReadWrite)
        .SetConstantBuffer("CB_Constant", 0)
        .SetExecutor(Bind<&ComputeScene::Demo16_Executor_>(this));

    m_graph.AddComputePass("Pass_GaussianBlurV")
        .SetProgram("P_GaussianBlurV")
        .SetTexture("T_TmpTexture", 0, eResourceAccess::ReadWrite)
        .SetTexture("T_OutTexture", 1, eResourceAccess::ReadWrite)
        .SetConstantBuffer("CB_Constant", 0)
        .SetExecutor(Bind<&ComputeScene::Demo16_Executor_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc blitDesc   = {};
    blitDesc.filter            = eImageFilter::Blit;
    blitDesc.input0Texture = "T_OutTexture";
    blitDesc.output            = eImageFilterOutput::Final;
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);

    m_bBool0 = true;   // apply
    m_float0 = 2.f;    // sigma of gaussian filter
    Demo16_UpdateGaussianFilter_();
}

void ComputeScene::Demo16_Executor_()
{
    struct CBConstant
    {
        UInt32 bApply;
        UInt32 padding[3];
        float  weights[8];
    };

    Graphics& gfx = GetGraphics();

    CBConstant data;
    data.bApply = m_bBool0;
    for (Int32 i = 0; i < 5; ++i)
    {
        data.weights[i] = m_kernel[i];
    }

    gfx.UpdateBuffer(m_graph.GetConstantBuffer("CB_Constant"), AsMemoryView(data));
    gfx.Dispatch((m_int0 + 15) / 16, (m_int1 + 15) / 16, 1);
}

void ComputeScene::Demo16_ImGui_()
{
    ImGui::Checkbox("Apply Blur", &m_bBool0);
    if (ImGui::SliderFloat("Sigma", &m_float0, 0.1f, 10.f))
    {
        Demo16_UpdateGaussianFilter_();
    }

    if (ImGui::Button("Capture"))
    {
        ExportTextureWithDialog_("T_OutTexture");
    }
}

void ComputeScene::Demo16_UpdateGaussianFilter_()
{
    CalcGaussianFilter1D({ m_kernel, 5 }, m_float0);
}

void ComputeScene::InitDemo17_GrayScaleHistogram_()
{
    Graphics& gfx = GetGraphics();

    // import image
    Expect<ImageImporter> importer = ImageImporter::ImportFromFile(SCENE_PATH("Image4.jpg"));
    JAM_ASSERT(importer, "Failed to load image.");
    const TextureHandle texh = gfx.CreateTexture(importer->GetImageContainer());
    const TextureInfo   texi = gfx.GetTextureInfo(texh);
    m_int0                   = texi.width;
    m_int1                   = texi.height;

    // make gradation texture (rgba8 unorm)
    Vector<UInt32> pixels(texi.width * texi.height, 0);
    for (Int32 y = 0; y < texi.height; ++y)
    {
        for (Int32 x = 0; x < texi.width; ++x)
        {
            const float c              = static_cast<float>(x) / static_cast<float>(texi.width);
            const RGBA  rgba           = RGBA::FromLinear(c, c, c);
            pixels[y * texi.width + x] = rgba.ToPackedABGR();
        }
    }
    MemoryView init[] = { pixels };

    m_graph.InsertTexture("T_Image", texh, true);
    m_graph.EmplaceTexture2D("T_Gradation", texi.width, texi.height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::ShaderReadWrite, init);
    m_graph.EmplaceStructuredBuffer("T_OutBuffer", 256, sizeof(UInt32), eStructureBuffer::Default, eStorageBufferFlags::ShaderReadWrite);

    LoadProgram(m_graph, "P_GrayScaleHistogram", SCENE_PATH("GrayScaleHistogramShader.hlsl"), eShaderFlags::Compute);

    m_graph.AddRenderPass("Pass_Clear")
        .AsSideEffect()
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f), 0);

    m_graph.AddComputePass("Pass_GrayScaleHistogram")
        .AsFinalPass()
        .SetProgram("P_GrayScaleHistogram")
        .SetBuffer("T_OutBuffer", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ComputeScene::Demo17_Executor_>(this));

    m_bBool0 = false;
    m_bBool1 = true;   // true: image. false: gradation
}

void ComputeScene::Demo17_Executor_()
{
    if (m_bBool0)
    {
        Graphics& gfx = GetGraphics();

        Vector<UInt32> zero(256, 0);
        gfx.UpdateBuffer(m_graph.GetStorageBuffer("T_OutBuffer"), zero, 0, false);

        if (m_bBool1)
        {
            // input image
            gfx.SetTexture(m_graph.GetTexture("T_Image"), eShader::ComputeShader, 0);
        }
        else
        {
            // gradation texture
            gfx.SetTexture(m_graph.GetTexture("T_Gradation"), eShader::ComputeShader, 0);
        }

        // dispatch
        gfx.Dispatch((m_int0 + 15) / 16, (m_int1 + 15) / 16, 1);

        const StorageBufferHandle src      = m_graph.GetStorageBuffer("T_OutBuffer");
        StorageBufferHandle       readback = gfx.CreateReadbackBuffer(256 * sizeof(UInt32));
        gfx.CopyBuffer(readback, 0, src, 0);

        Vector<UInt32> hist(256, 0);
        gfx.ReadBuffer(readback, hist);
        JAM_LOG_INFO("========== GrayScale Histogram ==========");
        if (m_bBool1)
        {
            JAM_LOG_INFO("Input: Image");
        }
        else
        {
            JAM_LOG_INFO("Input: Gradation");
        }
        for (Int32 i = 0; i < 256; ++i)
        {
            JAM_LOG_INFO("hist[{}] = {}", i, hist[i]);
        }
        JAM_LOG_INFO("========== Done ==========");

        JAM_DESTROY_STORAGE_BUFFER(readback);
        m_bBool0 = false;
    }
}

void ComputeScene::Demo17_ImGui_()
{
    ImGui::BeginGroup();
    {
        Widget_Image(m_graph.GetTexture("T_Image"), { 128, 128 });
        if (ImGui::RadioButton("Image", m_bBool1))
        {
            m_bBool1 = !m_bBool1;
        }
    }
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        Widget_Image(m_graph.GetTexture("T_Gradation"), { 128, 128 });
        if (ImGui::RadioButton("Gradation", !m_bBool1))
        {
            m_bBool1 = !m_bBool1;
        }
    }
    ImGui::EndGroup();

    if (ImGui::Button("Compute"))
    {
        m_bBool0 = true;

        Graphics& gfx = GetGraphics();
        gfx.TriggerDebuggerCapture();
    }
}

void ComputeScene::ExportTextureWithDialog_(
    const StringView _name) const
{
    Graphics& gfx = GetGraphics();

    FileDialogFilter filter[] = {
        { "PNG Image", "*.png" },
        { "All Files",   "*.*" }
    };

    SaveFileDialogInfo fdi = {};
    fdi.filtersOrEmpty     = filter;

    FileDialog fd { fdi };
    if (fd.Open())
    {
        const StringView    path = fd.GetFirstSelectedPath();
        const TextureHandle texh = m_graph.GetTexture(_name);
        if (ImageExporter::ExportToFile(texh, path, eImageFormat::PNG, true))
        {
            JAM_ASSERT(0, "Failed to export image.");
        }
    }
}