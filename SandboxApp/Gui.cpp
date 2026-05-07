#include "pch.h"
#include "Gui.h"

#include "SampleCollector.h"
#include "../JamEngine/Application.h"
#include "../JamEngine/EditorWidgets.h"
#include "../JamEngine/StackString.h"

bool BeginSceneInspectorWindows(const char* _title)
{
    if (!ImGui::Begin(_title))
    {
        return false;
    }

    // scene info
    ImGui::Text("Scene: %s", _title);

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Performance"))
    {
        static SampleCollector s_collector;
        static bool            s_bShowHistogram = true;

        Application& app  = GetApp();
        const float  dtMs = app.GetDeltaTimeSec() * 1000.f;
        s_collector.PushSample(dtMs);

        const float frameRate = app.GetFrameRate();
        const float avgDtMs   = s_collector.GetAverage();
        const float avgFps    = 1000.f / avgDtMs;

        StackString<256> buf;
        buf.Format("Delta time: {:.3f}\tms\n"
                   "Frame Rate: {:.3f}\tfps\n"
                   "Avg. Delta time: {:.3f}\tms\n"
                   "Avg. Frame Rate: {:.3f}\tfps",
                   dtMs,
                   frameRate,
                   avgDtMs,
                   avgFps);

        ImGui::Spacing();
        if (s_bShowHistogram)
        {
            ImGui::PlotLines("##FrameTimeHistogram", s_collector.GetCpuTimeSamples(), s_collector.GetSampleCount(), s_collector.GetOffset(), buf.GetPtr(), 0.f, 60.f, { 0, 80 });
        }
        else
        {
            ImGui::TextUnformatted(buf.GetPtr(), buf.GetTerminate());
        }

        ImGui::Spacing();
        if (ImGui::Checkbox("Show Frame Time Histogram", &s_bShowHistogram))
        {
            s_collector = SampleCollector();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Graphics Debug"))
    {
        Graphics&            gfx   = GetGraphics();
        const GraphicsStats& stats = gfx.GetStats();

        ImGui::Text("Submit");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Draw calls");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numDrawCalls);
            ImGui::TableNextColumn();
            ImGui::Text("Dispatch calls");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numDispatchCalls);
            ImGui::TableNextColumn();
            ImGui::Text("Resolve");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numResolve);

            ImGui::EndTable();
        }
        ImGui::Unindent();

        ImGui::Text("Program");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Program changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numProgramChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Compute program changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numComputeProgramChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();
        ImGui::Spacing();

        ImGui::Text("Input Assembler (IA)");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Primitive topology changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numPrimitiveTopologyChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Vertex buffer changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numVertexBufferChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Instance buffer changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numInstanceBufferChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Input layout changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numInputLayoutChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Index buffer changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numIndexBufferChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();
        ImGui::Spacing();

        ImGui::Text("Vertex Shader (VS)");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Constant Buffer");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numVSConstantBufferChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Read Resource");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numVSReadResourceChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Sampler State");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numVSSamplerStateChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();

        ImGui::Text("Hull Shader (HS)");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Constant Buffer");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numHSConstantBufferChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Read Resource");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numHSReadResourceChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Sampler State");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numHSSamplerStateChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();

        ImGui::Text("Domain Shader (DS)");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Constant Buffer");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numDSConstantBufferChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Read Resource");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numDSReadResourceChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Sampler State");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numDSSamplerStateChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();

        ImGui::Text("Geometry Shader (GS)");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Constant Buffer");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numGSConstantBufferChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Read Resource");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numGSReadResourceChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Sampler State");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numGSSamplerStateChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();

        ImGui::Text("Pixel Shader (PS)");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Constant Buffer");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numPSConstantBufferChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Read Resource");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numPSReadResourceChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Sampler State");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numPSSamplerStateChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Read/Write Resource");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numPSReadWriteResourceChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();

        ImGui::Text("Compute Shader (CS)");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Constant Buffer");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numCSConstantBufferChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Read Resource");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numCSReadResourceChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Sampler State");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numCSSamplerStateChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Read/Write Resource");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numCSReadWriteResourceChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();

        ImGui::Text("Rasterizer (RS)");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Rasterizer state changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numRasterizerStateChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Viewport changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numViewportChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Scissor rect changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numScissorRectChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();
        ImGui::Spacing();

        ImGui::Text("Output Merger (OM)");
        ImGui::Indent();
        if (ImGui::BeginTable("GraphicsStatsTable", 2))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::Text("Frame buffer changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numFrameBufferChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Blend state changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numBlendStateChanges);
            ImGui::TableNextColumn();
            ImGui::Text("Depth stencil state changes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", stats.numDepthStencilStateChanges);

            ImGui::EndTable();
        }
        ImGui::Unindent();

        ImGui::TreePop();
    }

    ImGui::Separator();

    // user gui..
    return true;
}