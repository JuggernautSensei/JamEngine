#include <algorithm>
#include <iomanip>
#include <ranges>
#include <sstream>

#include "../JamEngine/JamEngine.h"
using namespace jam;

#ifdef JAM_DEBUG
#    pragma comment(lib, "../JamEngine/bin/Debug/JamEngine.lib")
#else
#    pragma comment(lib, "../JamEngine/bin/Release/JamEngine.lib")
#endif

////////////////////////////////
///          STRUCT          ///
////////////////////////////////

struct Macro
{
    std::string name;
    std::string value;
};

struct ShaderItem
{
    std::string        source     = {};
    std::string        name       = {};
    fs::path           path       = {};
    eShader            stage      = eShader::VertexShader;
    std::string        entryPoint = "main";
    std::vector<Macro> macros     = {};
};

struct Project
{
    std::string             nameSpace   = "jam::shader";
    std::vector<ShaderItem> shaderItems = {};
};

///////////////////////////////////
///          JSON GLUE          ///
///////////////////////////////////

void to_json(Json& j, const Macro& macro)
{
    j["name"]  = macro.name;
    j["value"] = macro.value;
}

void from_json(const Json& j, Macro& macro)
{
    macro.name  = j.value<std::string>("name", "none");
    macro.value = j.value<std::string>("value", "none");
}

void to_json(Json& j, const ShaderItem& item)
{
    j["name"]       = item.name;
    j["path"]       = item.path.native();
    j["stage"]      = StringFrom(item.stage);
    j["entryPoint"] = item.entryPoint;
    j["macros"]     = item.macros;
}

void from_json(const Json& j, ShaderItem& item)
{
    item.name = j.value<std::string>("name", "none");
    item.path = j.value<std::string>("path", "none");

    // EnumFromString returns optional, need to handle it
    const std::string stageStr = j.value<std::string>("stage", "VertexShader");
    if (const auto optStage = EnumFrom<eShader>(stageStr))
    {
        item.stage = *optStage;
    }
    else
    {
        item.stage = eShader::VertexShader;
    }

    item.entryPoint = j.value<std::string>("entryPoint", "main");
    if (j.contains("macros") && j["macros"].is_array())
        item.macros = j["macros"].get<std::vector<Macro>>();
}

void to_json(Json& j, const Project& project)
{
    j["namespace"] = project.nameSpace;
    j["shaders"]   = project.shaderItems;
}

void from_json(const Json& j, Project& project)
{
    project.nameSpace = j.value<std::string>("namespace", "jam::shader");
    if (j.contains("shaders") && j["shaders"].is_array())
        project.shaderItems = j["shaders"].get<std::vector<ShaderItem>>();
}

///////////////////////////////////
///          UTILITIES          ///
///////////////////////////////////

static std::string MakeVariableName(std::string v)
{
    std::ranges::replace(v, ' ', '_');
    std::ranges::replace(v, '-', '_');
    return v;
}

static std::vector<ShaderMacro> MakeShaderMacros(const std::vector<Macro>& src)
{
    std::vector<ShaderMacro> dst;
    dst.reserve(src.size() + 1);
    for (size_t i = 0; i < src.size(); ++i)
    {
        const Macro& m = src[i];
        dst.push_back({ m.name.c_str(), m.value.c_str() });
    }

    if (dst.empty() || dst.back().IsNullTerminator() == false)
    {
        dst.push_back(ShaderMacro::NullTerminator());
    }
    return dst;
}

static void DrawCenteredNote(const char* msg)
{
    const ImVec2 curCursor = ImGui::GetCursorScreenPos();
    const ImVec2 textSize  = ImGui::CalcTextSize(msg);
    const ImVec2 winSize   = ImGui::GetWindowSize();
    const ImVec2 textStart = { (winSize.x - textSize.x) * 0.5f, (winSize.y - textSize.y) * 0.5f };
    ImGui::SetCursorScreenPos({ curCursor.x + textStart.x, curCursor.y + textStart.y });
    ImGui::TextDisabled(msg);
}

static void EmitBytecodeArrays(std::stringstream& _stream, const std::vector<ShaderItem>& _items, const bool _bIsDebug)
{
    const eShaderCompileFlags flags = _bIsDebug ? eShaderCompileFlags_DebugDefault : eShaderCompileFlags_Optimization;

    const Int32 itemCount = NarrowCast<Int32>(_items.size());
    for (Int32 idx = 0; idx < itemCount; ++idx)
    {
        const ShaderItem&        item   = _items[idx];
        std::vector<ShaderMacro> macros = MakeShaderMacros(item.macros);

        ShaderCompiler compiler;
        const bool           success = compiler.CompileFile(item.path, item.stage, item.entryPoint, flags, macros);
        if (!success)
        {
            Log::Error(R"(Failed to compile shader "{}". {})", item.name, GlobalBuffer::GetLastError());
            continue;
        }

        CMemView bytecode  = compiler.GetBytecode();
        const Byte*     pByte     = bytecode.GetPtr();
        const size_t          byteWidth = bytecode.GetByteWidth();
        std::string     varName   = MakeVariableName(item.name);

        _stream << "constexpr std::uint8_t " << varName << "_Bytecode" << "[] = {";
        for (size_t i = 0; i < byteWidth; ++i)
        {
            if (i % 12 == 0) _stream << "\n    ";
            _stream << "0x" << std::hex << std::setw(2) << std::setfill('0') << NarrowCast<int>(pByte[i]);
            if (i + 1 != byteWidth) _stream << ", ";
        }
        _stream << std::dec << "\n};\n"
                << "constexpr std::size_t " << varName << "_Size = " << byteWidth << ";\n\n";
    }
}

///////////////////////////////
///          SCENE          ///
///////////////////////////////

class WorkScene : public Scene
{
    JAM_SCENE(WorkScene);

public:
    void OnImGuiRender() override
    {
        ShowMainMenuBar();
        ShowWorkspace();
    }

private:
    void SaveProject(const fs::path& _path) const
    {
        // to json
        fs::path path = _path;
        path.replace_extension(".json");

        // save
        Json json    = m_project;
        const bool success = SaveJsonToFile(path, json);
        if (!success)
        {
            Log::Error(GlobalBuffer::GetLastError());
        }
    }

    void OpenProject(const fs::path& _path)
    {
        // clear existing
        m_project.shaderItems.clear();

        // load json
        const std::optional<Json> optJson = LoadJsonFromFile(_path);
        if (!optJson)
        {
            Log::Error(GlobalBuffer::GetLastError());
            return;
        }

        // project load from json
        m_project = optJson->get<Project>();

        // load shader sources
        for (ShaderItem& item: m_project.shaderItems)
        {
            std::optional<std::string> optSrc = LoadFromFile(item.path);
            if (!optSrc)
            {
                Log::Error(GlobalBuffer::GetLastError());
                continue;
            }
            item.source = std::move(*optSrc);
        }

        // done
        Log::Info(R"(Project loaded: "{}")", _path.native());
    }

    std::optional<ShaderItem> LoadShaderItem(const fs::path& path) const
    {
        ShaderItem item;
        item.path = path;

        // load source
        const std::optional<std::string> optSource = LoadFromFile(path);
        if (!optSource)
        {
            return std::nullopt;
        }

        item.source = std::move(*optSource);
        item.name   = path.stem().native();
        return item;
    }

    void SaveShaderItem(const ShaderItem& item) const
    {
        const bool success = SaveToFile(item.path, item.source);
        if (!success)
        {
            Log::Error(GlobalBuffer::GetLastError());
            return;
        }
    }

    void ExportAsSingleHeader(const fs::path& _path) const
    {
        // header path
        fs::path headerPath = _path;
        headerPath.replace_extension(".h");

        // emit
        std::stringstream header;
        header << "#pragma once\n\n"
               << "#include <cstddef>\n"
               << "#include <cstdint>\n\n"
               << "namespace " << m_project.nameSpace << " {\n\n";

        // for debug
        header << "#ifdef JAM_DEBUG\n";
        EmitBytecodeArrays(header, m_project.shaderItems, true);

        // for release
        header << "#else\n";
        EmitBytecodeArrays(header, m_project.shaderItems, false);

        // end
        header << "#endif\n\n"
               << "} // namespace jam::shader\n";

        // save
        const bool success = SaveToFile(headerPath, header.str());
        if (!success)
        {
            Log::Error(GlobalBuffer::GetLastError());
            return;
        }

        // done
        Log::Info(R"(Single header file saved: "{}")", headerPath.native());
    }

    ////////////////////////////
    ///          UI          ///
    ////////////////////////////

    void ShowWorkspace()
    {
        // DockSpace
        ImGuiID dockID = ImGui::DockSpaceOverViewport();

        // begin window
        ImGui::SetNextWindowDockID(dockID);
        if (!ImGui::Begin("workspace"))
        {
            ImGui::End();
            return;
        }

        // left: shader list
        ImGui::BeginGroup();
        {
            ImGui::TextUnformatted("shader list");

            // load button
            if (ImGui::Button("Load"))
            {
                constexpr FileDialogFilter k_filters[] = {
                    { "HLSL Files", "*.hlsl *.hlsli" },
                    {  "All Files",            "*.*" },
                };

                auto pathes = OpenMultiFileDialog("Load HLSL Shader", {}, k_filters);
                if (pathes)
                {
                    for (const fs::path& path: *pathes)
                    {
                        std::optional<ShaderItem> optShaderItem = LoadShaderItem(path);
                        if (optShaderItem)
                        {
                            m_project.shaderItems.push_back(std::move(*optShaderItem));
                            m_pCurrentShader = &m_project.shaderItems.back();
                            Log::Info(R"(Shader loaded: "{}")", path.native());
                        }
                        else
                        {
                            Log::Error(GlobalBuffer::GetLastError());
                        }
                    }
                }
            }

            // clone button
            ImGui::SameLine();
            ImGui::BeginDisabled(m_pCurrentShader == nullptr);
            if (ImGui::Button("Clone"))
            {
                if (m_pCurrentShader)
                {
                    m_project.shaderItems.push_back(*m_pCurrentShader);
                    m_pCurrentShader = &m_project.shaderItems.back();
                }
            }

            // delete button
            ImGui::SameLine();
            if (ImGui::Button("Delete"))
            {
                if (m_pCurrentShader)
                {
                    std::erase_if(m_project.shaderItems,
                                  [this](const ShaderItem& item)
                                  {
                                      return &item == m_pCurrentShader;
                                  });

                    m_pCurrentShader = nullptr;
                }
            }
            ImGui::EndDisabled();

            // render shader item list
            ImVec2 shaderListChildSize { 200.f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() };
            if (ImGui::BeginChild("shader list", shaderListChildSize, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX))
            {
                Int32 idx = 0;
                for (ShaderItem& item: m_project.shaderItems)
                {
                    ImGui::PushID(idx++);
                    if (ImGui::Selectable(item.name.c_str(), m_pCurrentShader == &item))
                    {
                        m_pCurrentShader = &item;
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
            ImGui::Text("total shader count: %zu", m_project.shaderItems.size());
        }
        ImGui::EndGroup();

        // shader option editor
        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            ImGui::TextUnformatted("shader option");

            ImVec2 shaderOptionEditorChildSize { 300.f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() };
            if (ImGui::BeginChild("shader editor", shaderOptionEditorChildSize, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX))
            {
                if (m_pCurrentShader)
                {
                    ShaderItem& item = *m_pCurrentShader;

                    if (ImGui::BeginTable("shader option editor table", 2))
                    {
                        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 100);
                        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

                        // name
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("name: ");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputText("##name", &item.name);

                        // path
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("path: ");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        std::string pathStr = item.path.native();
                        ImGui::InputText("##path", &pathStr, ImGuiInputTextFlags_ReadOnly);

                        // entry point
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("entry point: ");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputText("##entrypoint", &item.entryPoint);

                        // stage
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("shader stage: ");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        if (ImGui::BeginCombo("##stage", StringFrom(item.stage).data()))
                        {
                            for (eShader s: EnumRanges<eShader>())
                            {
                                const bool selected = (item.stage == s);
                                if (ImGui::Selectable(StringFrom(s).data(), selected))
                                    item.stage = s;
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::EndTable();
                    }

                    // macros
                    ImGui::Separator();
                    ImGui::TextUnformatted("macros:");
                    ImVec2 macroListChildSize = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() };
                    if (ImGui::BeginChild("macro list", macroListChildSize))
                    {
                        float availWidth        = ImGui::GetContentRegionAvail().x;
                        float labelColumnWidth  = 50.f;
                        float deleteColumnWidth = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x * 2.f;
                        float inputColumnWidth  = (availWidth - labelColumnWidth * 2.f - deleteColumnWidth - ImGui::GetStyle().ItemSpacing.x * 4.f) * 0.5f;

                        if (ImGui::BeginTable("macro list table", 5))
                        {
                            ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, labelColumnWidth);
                            ImGui::TableSetupColumn("##nameInput", ImGuiTableColumnFlags_WidthFixed, inputColumnWidth);
                            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, labelColumnWidth);
                            ImGui::TableSetupColumn("##valueInput", ImGuiTableColumnFlags_WidthFixed, inputColumnWidth);
                            ImGui::TableSetupColumn("##delete", ImGuiTableColumnFlags_WidthFixed, deleteColumnWidth);

                            Int32 macroCount = NarrowCast<Int32>(item.macros.size());
                            for (Int32 i = 0; i < macroCount; ++i)
                            {
                                Macro& macro = item.macros[i];
                                ImGui::PushID(i);
                                ImGui::TableNextRow();

                                // name label
                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted("name:");

                                // name input
                                ImGui::TableNextColumn();
                                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                                ImGui::InputText("##name", &macro.name);

                                // value label
                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted("value:");

                                // value input
                                ImGui::TableNextColumn();
                                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                                ImGui::InputText("##value", &macro.value);

                                // delete button
                                ImGui::TableNextColumn();
                                if (ImGui::Button("X"))
                                {
                                    item.macros.erase(item.macros.begin() + i);
                                    if (i > 0) { --i; }
                                }
                                ImGui::PopID();
                            }
                            ImGui::EndTable();
                        }
                    }
                    ImGui::EndChild();

                    if (ImGui::Button("Add Macro"))
                    {
                        item.macros.emplace_back();
                    }
                }
                else
                {
                    DrawCenteredNote("No shader selected.\nPlease select a shader from the list on the left.");
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndGroup();

        // source source editor
        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            ImGui::TextUnformatted("shader source");

            ImVec2 shaderSourceChildSize = ImGui::GetContentRegionAvail();
            if (ImGui::BeginChild("shader source", shaderSourceChildSize, ImGuiChildFlags_Borders))
            {
                if (m_pCurrentShader)
                {
                    ShaderItem& item = *m_pCurrentShader;

                    if (ImGui::Button("compile as debug"))
                    {
                        bool success = CompileShader(item, true);
                        if (!success)
                        {
                            Log::Error(R"(Failed to compile shader "{}". {})", item.name, GlobalBuffer::GetLastError());
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("compile as release"))
                    {
                        bool success = CompileShader(item, false);
                        if (!success)
                        {
                            Log::Error(R"(Failed to compile shader "{}". {})", item.name, GlobalBuffer::GetLastError());
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("save source"))
                    {
                        SaveShaderItem(item);
                    }

                    ImVec2 shaderSourceEditorChildSize = ImGui::GetContentRegionAvail();
                    if (ImGui::BeginChild("shader source editor", shaderSourceEditorChildSize, ImGuiChildFlags_Borders))
                    {
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputTextMultiline("##source", &item.source, ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_AllowTabInput);
                    }
                    ImGui::EndChild();
                }
                else
                {
                    DrawCenteredNote("No shader selected.\nPlease select a shader from the list on the left.");
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndGroup();
        ImGui::End();
    }

    void ShowMainMenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("file"))
            {
                if (ImGui::MenuItem("new project", "Ctrl+N"))
                {
                    m_project.shaderItems.clear();
                    m_pCurrentShader = nullptr;
                }

                ImGui::SetNextItemShortcut(ImGuiMod_Ctrl, 'O');
                if (ImGui::MenuItem("open project", "Ctrl+O"))
                {
                    constexpr FileDialogFilter k_filters[] = {
                        { "JSON Files", "*.json" },
                        {  "All Files",    "*.*" }
                    };

                    const std::optional<fs::path> pathOpt = OpenFileDialog("Open Project", {}, k_filters);
                    if (pathOpt)
                    {
                        OpenProject(pathOpt.value());
                    }
                }

                ImGui::SetNextItemShortcut(ImGuiMod_Ctrl, 'S');
                if (ImGui::MenuItem("save project", "Ctrl+S"))
                {
                    constexpr FileDialogFilter k_filters[] = {
                        { "JSON Files", "*.json" },
                        {  "All Files",    "*.*" }
                    };

                    const std::optional<fs::path> pathOpt = SaveFileDialog("Save Project", {}, k_filters);
                    if (pathOpt)
                    {
                        SaveProject(pathOpt.value());
                    }
                }

                if (ImGui::MenuItem("export as single header"))
                {
                    constexpr FileDialogFilter k_filters[] = {
                        { "Header Files", "*.h" },
                        {    "All Files", "*.*" }
                    };
                    const std::optional<fs::path> pathOpt = SaveFileDialog("Export as C++ Source", {}, k_filters);
                    if (pathOpt)
                    {
                        ExportAsSingleHeader(pathOpt.value());
                    }
                }

                ImGui::Separator();
                if (ImGui::MenuItem("exit"))
                {
                    Application& app = Application::GetInstance();
                    app.Quit();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("edit"))
            {
                if (ImGui::BeginMenu("edit namespace"))
                {
                    ImGui::Text("this will be the namespace of the generated header file.");
                    ImGui::InputText("namespace", &m_project.nameSpace);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    bool CompileShader(const ShaderItem& item, const bool bDebug) const
    {
        ShaderCompiler compiler;

        const std::vector<ShaderMacro> macros = MakeShaderMacros(item.macros);
        const eShaderCompileFlags      flags  = bDebug ? eShaderCompileFlags_DebugDefault : eShaderCompileFlags_Optimization;

        const bool success = compiler.CompileFile(item.path, item.stage, item.entryPoint, flags, macros);
        if (!success)
        {
            return false;
        }

        Log::Info("Shader compiled successfully.");
        return true;
    }

    Project     m_project;
    ShaderItem* m_pCurrentShader = nullptr;
};

/////////////////////////////////////
///          APPLICATION          ///
/////////////////////////////////////

class MyApp : public Application
{
public:
    explicit MyApp(const ApplicationCreateInfo& info)
        : Application(info)
    {
    }

    void OnCreate() override
    {
        std::locale::global(std::locale::classic());

        const Ref<Scene> pScene = MakeRef<WorkScene>();
        SceneManager::LoadScene(pScene);

        Application&     app    = Application::GetInstance();
        WindowsPlatform& window = app.GetPlatform();
        window.ResizeClient(1280, 720);
    }

    void OnDestroy() override {}
};

Application* CreateApplication(const CommandLineArguments& _args)
{
    ApplicationCreateInfo info;
    info.applicationName = "JamShader";
    return new MyApp(info);
}

JAM_MAIN();
