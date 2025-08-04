#include <fstream>

#include "../JamEngine/JamEngine.h"
using namespace jam;

#ifdef _DEBUG
#pragma comment(lib, "../JamEngine/bin/Debug/JamEngine")
#else
#pragma comment(lib, "../JamEngine/bin/Release/JamEngine")
#endif

static FileDialogFilter k_jsonFilter[] = { {"JSON files", "pJson"} };
static FileDialogFilter k_hlslFilter[] = { {"HLSL files", "hlsl, hlsli"} };

struct MyShaderMacro
{
	std::string name;
	std::string value;
};

enum class eCompileStatus
{
	NotCompiled,
	Success,
	Failure
};

struct ShaderCompileData
{
	Json Serialize() const
	{
		Json json;
		json["name"] = name;
		json["filename"] = filename;
		json["entryPoint"] = entryPoint;
		json["target"] = target;
		Json macrosJson = Json::array();
		for (const MyShaderMacro& macro : macros)
		{
			Json macroJson;
			macroJson["name"] = macro.name;
			macroJson["value"] = macro.value;
			macrosJson.push_back(macroJson);
		}
		json["macros"] = macrosJson;
		return json;
	}

	void Deserialize(const Json& json)
	{
		name = json.value("name", "none");
		filename = json.value("filename", "none");
		entryPoint = json.value("entryPoint", "main");
		target = json.value("target", "none");
		macros.clear();
		if (json.contains("macros") && json["macros"].is_array())
		{
			for (const Json& macroJson : json["macros"])
			{
				MyShaderMacro& macro = macros.emplace_back();
				macro.name = macroJson.value("name", "");
				macro.value = macroJson.value("value", "");
			}
		}
		LoadFileContent();
	}

	void LoadFileContent()
	{
		std::ifstream ifs(filename);
		if (ifs.is_open())
		{
			fileContent.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		}
		else
		{
			fileContent.clear();
		}
		compileStatus = eCompileStatus::NotCompiled;
	}

	bool Compile()
	{
		std::vector<ShaderMacro> macrosVec;
		for (const auto& macro : macros)
		{
			macrosVec.emplace_back(macro.name, macro.value);
		}

		ShaderCompiler compiler;

		// Debug
		if (!compiler.CompileHLSLFromFile(filename, entryPoint, target, macrosVec, eShaderCompileOption::Debug))
		{
			compileStatus = eCompileStatus::Failure;
			return false;
		}
		compiler.GetCompiledShader(compiledBlobDebug.ReleaseAndGetAddressOf());

		// Optimized
		if (!compiler.CompileHLSLFromFile(filename, entryPoint, target, macrosVec, eShaderCompileOption::Optimized))
		{
			compileStatus = eCompileStatus::Failure;
			return false;
		}
		compiler.GetCompiledShader(compiledBlobOptimized.ReleaseAndGetAddressOf());

		compileStatus = eCompileStatus::Success;
		return true;
	}

	std::string name = "none";
	std::string filename = "none";
	std::string entryPoint = "main";
	std::string target = "none";
	std::vector<MyShaderMacro> macros;
	std::string fileContent;

	eCompileStatus compileStatus = eCompileStatus::NotCompiled;
	ComPtr<ID3DBlob> compiledBlobDebug;
	ComPtr<ID3DBlob> compiledBlobOptimized;
};

static ImVec2 CreateFitButtonSize(const Int32 _count = 1)
{
	JAM_ASSERT(_count > 0, "Button count must be greater than 0");
	float x = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * (_count - 1)) / static_cast<float>(_count);
	return { x, 0.f };
}

class MyScene : public Scene
{
public:
	explicit MyScene(const std::string_view& _name)
		: Scene(_name)
	{
	}

	void OnEnter() override
	{
		CreateRenderingResource();
	}

	void OnUpdate(float /*_deltaTime*/) override
	{
	}

	void OnRenderUI() override
	{
		float clearColor[4] = { 0.f, 0.f, 0.f, 1.0f };
		m_backBufferTexture.BindAsRenderTarget();
		m_backBufferTexture.ClearRenderTarget(clearColor);

		ImGui::Begin("shader compiler");

		RenderMainButtons();
		RenderShaderDataList();

		ImGui::End();
	}

	void OnEvent(Event& _eventRef) override
	{
		if (_eventRef.GetHash() == HashOf<WindowResizeEvent>())
		{
			CreateRenderingResource();
		}

		if (_eventRef.GetHash() == HashOf<BackBufferCleanupEvent>())
		{
			m_backBufferTexture.Reset();
		}
	}

	void CreateRenderingResource()
	{
		m_backBufferTexture = Renderer::GetBackBufferTexture();
		m_backBufferTexture.AttachRTV();
	}

private:
	void RenderMainButtons();
	void RenderShaderDataList();
	void RenderShaderDataItem(ShaderCompileData& _data, size_t _index);
	void RenderShaderDataMacros(ShaderCompileData& _data);

	void SaveConfiguration(const fs::path& _path) const;
	void LoadConfiguration(const fs::path& _path);

	void CompileAllShaders();
	void WriteCompiledShadersToFiles(const fs::path& _outputPath) const;
	void WriteShaderBlob(std::ofstream& _file, const std::string& _shaderName, ID3DBlob* _blob) const;
	static const char* GetCompileStatusText(eCompileStatus status);

	std::vector<ShaderCompileData> m_shaderCompileData;
	Texture2D m_backBufferTexture;
};

void MyScene::RenderMainButtons()
{
	ImVec2 buttonSize = CreateFitButtonSize(2);
	if (ImGui::Button("save config", buttonSize))
	{
		if (const fs::path path = GetApplication().GetWindow().SaveFileDialog(k_jsonFilter); !path.empty())
		{
			SaveConfiguration(path);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("load config", buttonSize))
	{
		if (const auto paths = GetApplication().GetWindow().OpenFileDialog(false, k_jsonFilter); !paths.empty())
		{
			LoadConfiguration(paths[0]);
		}
	}

	buttonSize = CreateFitButtonSize(2);
	if (ImGui::Button("Compile All", buttonSize))
	{
		CompileAllShaders();
	}

	ImGui::SameLine();
	if (ImGui::Button("Save Compiled Shaders", buttonSize))
	{
		if (fs::path outputPath = GetApplication().GetWindow().SaveFileDialog(); !outputPath.empty())
		{
			WriteCompiledShadersToFiles(outputPath);
			Log::Info("컴파일된 셰이더 저장 완료");
		}
	}
}

void MyScene::RenderShaderDataList()
{
	ImGui::Separator();
	ImGui::Text("shader compile data");
	if (ImGui::Button("add data", CreateFitButtonSize()))
	{
		m_shaderCompileData.emplace_back();
	}
	ImGui::Separator();

	for (size_t i = 0; i < m_shaderCompileData.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));
		RenderShaderDataItem(m_shaderCompileData[i], i);
		ImGui::PopID();
		ImGui::Separator();
	}
}

void MyScene::RenderShaderDataItem(ShaderCompileData& _data, const size_t _index)
{
	if (ImGui::BeginTable("shader compile data table", 3))
	{
		// name
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("name: ");
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##name", &_data.name);

		// Filename
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("filename: ");
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##filename", &_data.filename, ImGuiInputTextFlags_ReadOnly);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(_data.filename.c_str());
		}
		ImGui::TableSetColumnIndex(2);
		if (ImGui::Button("open file dialog", CreateFitButtonSize()))
		{
			if (const auto paths = GetApplication().GetWindow().OpenFileDialog(false, k_hlslFilter); !paths.empty())
			{
				_data.name = paths[0].stem().string();
				_data.name[0] = static_cast<char>(std::tolower(_data.name[0]));
				_data.filename = paths[0].string();
				_data.LoadFileContent();
			}
		}

		// Entry Point
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("entry point: ");
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##entryPoint", &_data.entryPoint);
		ImGui::TableSetColumnIndex(2);
		ImGui::Button("template##entrypoint", CreateFitButtonSize());
		if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonLeft))
		{
			if (ImGui::MenuItem("main")) _data.entryPoint = "main";
			if (ImGui::MenuItem("VSmain")) _data.entryPoint = "VSmain";
			if (ImGui::MenuItem("PSmain")) _data.entryPoint = "PSmain";
			if (ImGui::MenuItem("CSmain")) _data.entryPoint = "CSmain";
			if (ImGui::MenuItem("GSmain")) _data.entryPoint = "GSmain";
			if (ImGui::MenuItem("HSmain")) _data.entryPoint = "HSmain";
			if (ImGui::MenuItem("DSmain")) _data.entryPoint = "DSmain";
			ImGui::EndPopup();
		}

		// Target
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("target: ");
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##target", &_data.target);
		ImGui::TableSetColumnIndex(2);
		ImGui::Button("template##target", CreateFitButtonSize());
		if (ImGui::BeginPopupContextItem("target_template_popup", ImGuiPopupFlags_MouseButtonLeft))
		{
			if (ImGui::MenuItem("vs_5_0")) _data.target = "vs_5_0";
			if (ImGui::MenuItem("ps_5_0")) _data.target = "ps_5_0";
			if (ImGui::MenuItem("cs_5_0")) _data.target = "cs_5_0";
			if (ImGui::MenuItem("gs_5_0")) _data.target = "gs_5_0";
			if (ImGui::MenuItem("hs_5_0")) _data.target = "hs_5_0";
			if (ImGui::MenuItem("ds_5_0")) _data.target = "ds_5_0";
			ImGui::EndPopup();
		}

		RenderShaderDataMacros(_data);

		// Compile Status
		constexpr ImVec4 k_colorTable[3] =
		{
			ImVec4(0.5f, 0.5f, 0.5f, 1.0f), // Not compiled
			ImVec4(0.0f, 1.0f, 0.0f, 1.0f), // Success
			ImVec4(1.0f, 0.0f, 0.0f, 1.0f)  // Failure
		};

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Status:");
		ImGui::TableSetColumnIndex(1);
		ImGui::TextColored(k_colorTable[static_cast<int>(_data.compileStatus)], GetCompileStatusText(_data.compileStatus));
		ImGui::TableSetColumnIndex(2);
		if (ImGui::Button("Compile", CreateFitButtonSize()))
		{
			if (_data.Compile())
			{
				Log::Info("Shader [{}] compiled successfully.", _data.filename.c_str());
			}
			else
			{
				Log::Error("Failed to compile shader: {}", _data.filename.c_str());
			}
		}
		ImGui::EndTable();
	}

	ImVec2 buttonSize = CreateFitButtonSize(3);
	if (ImGui::Button("clone this data", buttonSize))
	{
		ShaderCompileData data = _data; 
		data.compiledBlobDebug.Reset();
		data.compiledBlobOptimized.Reset();
		data.compileStatus = eCompileStatus::NotCompiled;
		m_shaderCompileData.push_back(data);
	}

	ImGui::SameLine();
	if (ImGui::Button("remove this data", buttonSize))
	{
		m_shaderCompileData.erase(m_shaderCompileData.begin() + _index);
	}

	ImGui::SameLine();
	if (ImGui::Button("view source", buttonSize))
	{
		ImGui::OpenPopup("source code");
	}

	if (ImGui::BeginPopup("source code", ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputTextMultiline("##sourceCode", &_data.fileContent, ImVec2{ 400.f, 400.f }, ImGuiInputTextFlags_ReadOnly);
		ImGui::EndPopup();
	}
}

void MyScene::RenderShaderDataMacros(ShaderCompileData& _data)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();
	ImGui::Text("macros: ");

	ImGui::TableSetColumnIndex(1);
	if (ImGui::Button("add macro", CreateFitButtonSize()))
	{
		_data.macros.emplace_back();
	}

	ImGui::TableSetColumnIndex(2);
	if (ImGui::Button("clear macros", CreateFitButtonSize()))
	{
		_data.macros.clear();
	}

	float inputTextWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
	for (size_t j = 0; j < _data.macros.size(); ++j)
	{
		ImGui::PushID(static_cast<int>(j));
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);

		ImGui::SetNextItemWidth(inputTextWidth);
		ImGui::InputText("##name", &_data.macros[j].name);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(inputTextWidth);
		ImGui::InputText("##value", &_data.macros[j].value);

		ImGui::TableSetColumnIndex(2);
		if (ImGui::Button("remove", CreateFitButtonSize()))
		{
			_data.macros.erase(_data.macros.begin() + j);
			--j;
		}
		ImGui::PopID();
	}
}

void MyScene::SaveConfiguration(const fs::path& _path) const
{
	Json js = Json::array();
	for (const auto& data : m_shaderCompileData)
	{
		js.push_back(data.Serialize());
	}

	std::ofstream file(_path);
	if (file.is_open())
	{
		file << js.dump(4); // Pretty print with 4 spaces
	}
}

void MyScene::LoadConfiguration(const fs::path& _path)
{
	std::ifstream file(_path);
	if (file.is_open())
	{
		Json js;
		file >> js;
		m_shaderCompileData.clear();
		for (const auto& dataJson : js)
		{
			ShaderCompileData& data = m_shaderCompileData.emplace_back();
			data.Deserialize(dataJson);
		}
	}
}

void MyScene::CompileAllShaders()
{
	int successCount = 0;
	int failureCount = 0;
	for (auto& data : m_shaderCompileData)
	{
		if (data.Compile())
		{
			successCount++;
		}
		else
		{
			failureCount++;
		}
	}
	Log::Info("Compile All Finished. Success: %d, Failure: %d", successCount, failureCount);
}

void MyScene::WriteCompiledShadersToFiles(const fs::path& _outputPath) const
{
	std::vector<const ShaderCompileData*> successfullyCompiled;
	for (const auto& data : m_shaderCompileData)
	{
		if (data.compileStatus == eCompileStatus::Success)
		{
			successfullyCompiled.push_back(&data);
		}
	}

	if (successfullyCompiled.empty())
	{
		Log::Warn("No successfully compiled shaders to save.");
		return;
	}

	fs::path headerPath = _outputPath;
	headerPath.replace_extension(".h");

	std::ofstream headerFile(headerPath);
	if (!headerFile.is_open())
	{
		JAM_CRASH("Failed to open header file for writing: %s", headerPath.string().c_str());
	}

	headerFile << "// Auto-generated shader header file\n";
	headerFile << "// Compiled shaders count: " << successfullyCompiled.size() << "\n\n";
	headerFile << "#pragma once\n\n";

	for (const auto* data : successfullyCompiled)
	{
		std::string shaderName = data->name.empty() ? fs::path(data->filename).stem().string() : data->name;
		headerFile << "extern const unsigned char k_" << shaderName << "[];\n";
		headerFile << "extern const size_t k_" << shaderName << "Size;\n\n";
	}
	headerFile.close();

	fs::path sourcePath = _outputPath;
	sourcePath.replace_extension(".cpp");
	std::ofstream sourceFile(sourcePath);
	if (!sourceFile.is_open())
	{
		JAM_CRASH("Failed to open source file for writing: %s", sourcePath.string().c_str());
	}

	sourceFile << "// Auto-generated shader source file\n";
	sourceFile << "#include \"" << headerPath.filename().string() << "\"\n\n";

	for (const auto* data : successfullyCompiled)
	{
		std::string shaderName = data->name.empty() ? fs::path(data->filename).stem().string() : data->name;
		sourceFile << "#ifdef _DEBUG\n";
		WriteShaderBlob(sourceFile, shaderName, data->compiledBlobDebug.Get());
		sourceFile << "#else\n";
		WriteShaderBlob(sourceFile, shaderName, data->compiledBlobOptimized.Get());
		sourceFile << "#endif // _DEBUG\n\n";
	}
	sourceFile.close();
}

void MyScene::WriteShaderBlob(std::ofstream& _file, const std::string& _shaderName, ID3DBlob* _blob) const
{
	const auto* data = static_cast<const unsigned char*>(_blob->GetBufferPointer());
	const size_t size = _blob->GetBufferSize();

	_file << "const unsigned char k_" << _shaderName << "[] = {\n";
	for (size_t i = 0; i < size; ++i)
	{
		_file << "0x" << std::hex << static_cast<int>(data[i]);
		if (i < size - 1) _file << ", ";
		if ((i + 1) % 16 == 0) _file << "\n";
	}
	_file << "\n};\n";
	_file << "const size_t k_" << _shaderName << "Size = sizeof(k_" << _shaderName << ");\n";
}

const char* MyScene::GetCompileStatusText(const eCompileStatus status)
{
	switch (status)
	{
	case eCompileStatus::NotCompiled: return "not compiled";
	case eCompileStatus::Success: return "compile succeeded";
	case eCompileStatus::Failure: return "compile fail";
	}
	return "unknown";
}

class MyApplication : public Application
{
public:
	explicit MyApplication(const ApplicationCreateInfo& _info)
		: Application(_info)
	{
	}

	void OnCreate() override
	{
		SceneLayer* pSceneLayer = GetSceneLayer();
		pSceneLayer->AddScene(std::make_unique<MyScene>("My Scene"));
		pSceneLayer->ChangeScene("My Scene");
	}

	void OnDestroy() override
	{
	}
};

Application* CreateApplication(MAYBE_UNUSED const CommandLineArguments& _args)
{
	return new MyApplication({ "Shader Compiler", fs::current_path() });
}

JAM_MAIN();