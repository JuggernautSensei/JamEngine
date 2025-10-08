#include "../JamEngine/JamEngine.h"
using namespace jam;

#ifdef JAM_DEBUG
#    pragma comment(lib, "../JamEngine/bin/Debug/JamEngine")
#else
#    pragma comment(lib, "../JamEngine/bin/Release/JamEngine")
#endif

#include <fstream>

constexpr FileDialogFilter k_imageFilter[] = {
    { "image files", "png, jpg, jpeg, bmp, tga, hdr, exr, gif, bmp" }
};
constexpr eImageFormat k_saveImageFormat = eImageFormat::DDS;

struct Image
{
    void Initialize(const fs::path& _filePath)
    {
        Clear();
        displayPath = _filePath.string();
        if (texture.LoadFromFile(displayPath))
        {
            texture.AttachSRV();
            displayName = _filePath.stem().string();
        }
    }

    void        Clear() { *this = {}; }
    std::string displayPath = {};
    std::string displayName     = {};
    TextureRef   texture  = {};   // texture object
};

class MyScene : public Scene
{
public:
    explicit MyScene(const std::string_view _name)
        : Scene(_name)
    {
        const Window& window = Application::GetInstance().GetWindow();
        auto [width, height] = window.GetWindowSize();
        CreateRenderingResource(width, height);
    }

    ~MyScene() override = default;

    MyScene(const MyScene&)                = delete;
    MyScene& operator=(const MyScene&)     = delete;
    MyScene(MyScene&&) noexcept            = default;
    MyScene& operator=(MyScene&&) noexcept = default;

    bool SaveBuiltInImageSource(const fs::path& path) const
    {
        fs::path headerPath = path;
        headerPath.replace_extension(".h");

        fs::path sourcePath = path;
        sourcePath.replace_extension(".cpp");

        if (!WriteImageHeaderFile(headerPath))
        {
            return false;
        }

        if (!WriteImageSourceFile(headerPath, sourcePath))
        {
            return false;
        }

        log::Info("Built-in image source code saved successfully.");
        return true;
    }

    void OnImGuiRender() override
    {
        float clearColor[4] = { 0.f, 0.f, 0.f, 1.0f };
        m_backBufferTexture.BindAsRenderTarget();
        m_backBufferTexture.ClearRenderTarget(clearColor);

        ImGuiID id = ImGui::DockSpaceOverViewport();
        ImGui::SetNextWindowDockID(id, ImGuiCond_Always);
        if (ImGui::Begin("built-in image source creator"))
        {
            RenderImageListPanel();
            RenderSelectedImagePanel();
        }
        ImGui::End();
    }

    void OnEvent(Event& _event) override
    {
        if (_event.GetHash() == HashOf<WindowResizeEvent>())
        {
            const WindowResizeEvent& event = static_cast<const WindowResizeEvent&>(_event);
            CreateRenderingResource(event.GetWidth(), event.GetHeight());
        }

        if (_event.GetHash() == HashOf<BackBufferCleanupEvent>())
        {
            m_backBufferTexture.Reset();
        }
    }

    void CreateRenderingResource(UInt32 _width, UInt32 _height)
    {
        m_backBufferTexture.InitializeFromSwapchain(D3D11API::GetSwapchain());
        m_backBufferTexture.AttachRTV();
    }

private:
    void RenderImageListPanel()
    {
        ImGui::Text("image list (image count: %zu)", m_images.size());

        // image list
        if (ImGui::BeginChild("image list child", ImVec2 { 0.f, 200.f }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY))
        {
            static ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(m_images.size()));
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                {
                    Image& image = m_images[i];
                    if (ImGui::Selectable(image.displayPath.c_str(), m_pSelectedImage == &image))
                    {
                        m_pSelectedImage = &image;
                    }
                }
            }
        }
        ImGui::EndChild();

        // buttons
        ImVec2 buttonSize;
        buttonSize.x = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.f;

        // load button
        if (ImGui::Button("load", buttonSize))
        {
            auto [pathes, bResult] = OpenFileDialogMulti(k_imageFilter);
            if (bResult)
            {
                for (const fs::path& path: pathes)
                {
                    Image image;
                    image.Initialize(path);
                    m_images.push_back(std::move(image));
                }
            }
        }

        // delete button
        ImGui::SameLine();
        ImGui::BeginDisabled(m_pSelectedImage == nullptr);
        if (ImGui::Button("delete", buttonSize))
        {

            auto it = std::ranges::find_if(m_images,
                                           [this](const Image& _image)
                                           {
                                               return _image.displayPath == m_pSelectedImage->displayPath;
                                           });

            it               = m_images.erase(it);
            m_pSelectedImage = (it == m_images.end()) ? nullptr : &*it;
        }
        ImGui::EndDisabled();

        // save to memory button
        buttonSize.x = ImGui::GetContentRegionAvail().x;
        if (ImGui::Button("save built-in image source code in file", buttonSize))
        {
            auto [path, bResult] = SaveFileDialog();
            if (bResult)
            {
                if (SaveBuiltInImageSource(path))   // 성공
                {
                    log::Info("Built-in image source code saved to file: {}", path.string());
                }
                else   // 실패
                {
                    JAM_ASSERT(0, "Failed to save built-in image source code to file: {}", path.string());
                }
            }
        }
    }

    void RenderSelectedImagePanel() const
    {
        if (!m_pSelectedImage)
        {
            ImGui::Text("no image loaded.");
            return;
        }

        Image& imageRef = *m_pSelectedImage;

        ImGui::BulletText("texture info");
        if (ImGui::BeginTable("texture info table", 2, ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("file path:");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", imageRef.displayPath.c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("label:");
            ImGui::TableSetColumnIndex(1);
            ImGui::InputText("##label", &imageRef.displayName, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

            auto [width, height] = imageRef.texture.GetSize();
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("width:");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", width);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("height:");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", height);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("formats:");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", EnumToString(imageRef.texture.GetFormat()).data());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("array size:");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", imageRef.texture.GetArraySize());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("sample count:");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", imageRef.texture.GetSampleCount());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("mips:");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", imageRef.texture.HasMips() ? "true" : "false");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("cubemap:");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", imageRef.texture.IsCubemap() ? "true" : "false");

            ImGui::EndTable();
        }

        {
            constexpr float k_imagePreviewHeight = 300.f;
            static float    s_imagePreviewScale  = 1.f;
            auto [width, height]                 = imageRef.texture.GetSize();

            ImGui::BulletText("texture preview");
            ImGui::SliderFloat("scale", &s_imagePreviewScale, 0.1f, 10.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

            float imageWidth  = k_imagePreviewHeight * s_imagePreviewScale;
            float imageHeight = (imageWidth * height) / width;
            ImGui::Image(reinterpret_cast<ImTextureID>(imageRef.texture.GetSRV()), ImVec2 { imageWidth, imageHeight });
        }
    }

    bool WriteImageHeaderFile(const fs::path& headerPath) const
    {
        log::Info("Saving built-in image header to: {}", headerPath.string());

        std::ofstream fs { headerPath, std::ios::out };
        if (!fs)
        {
            JAM_ASSERT(0, "Failed to open file for writing: {}", headerPath.string());
            return false;
        }

        fs << "#pragma once\n\n";
        fs << "// this file is auto-generated by ImageLoader application.\n";
        fs << "// image count: " << m_images.size() << "\n\n";
        for (const Image& image: m_images)
        {
            log::Info("Saving image: {} (size: {}x{}, formats: {})",
                      image.displayName,
                      image.texture.GetSize().first,
                      image.texture.GetSize().second,
                      EnumToString(image.texture.GetFormat()));

            fs << "extern unsigned char k_" << image.displayName << EnumToString(k_saveImageFormat) << "[];\n";
            fs << "extern size_t k_" << image.displayName << EnumToString(k_saveImageFormat) << "Size;\n\n";
        }
        return true;
    }

    bool WriteImageSourceFile(const fs::path& headerPath, const fs::path& sourcePath) const
    {
        log::Info("Saving built-in image source to: {}", sourcePath.string());

        std::ofstream fs { sourcePath, std::ios::out };
        fs.imbue(std::locale::classic());
        if (!fs)
        {
            JAM_ASSERT(0, "Failed to open file for writing: {}", sourcePath.string());
            return false;
        }

        fs << "#include \"" << headerPath.filename().string() << "\"\n\n";
        fs << "// this file is auto-generated by ImageLoader application.\n";
        fs << "// image count: " << m_images.size() << "\n\n";
        for (const Image& image: m_images)
        {
            log::Info("Saving image: {} (size: {}x{}, formats: {})",
                      image.displayName,
                      image.texture.GetSize().first,
                      image.texture.GetSize().second,
                      EnumToString(image.texture.GetFormat()));

            auto [blob, bResult] = image.texture.SaveToMemory(k_saveImageFormat);
            if (bResult)
            {
                fs << "unsigned char k_" << image.displayName << EnumToString(k_saveImageFormat) << "[] = {";
                for (size_t i = 0; i < blob.size(); ++i)
                {
                    fs << static_cast<int>(blob[i]);
                    if (i < blob.size() - 1)
                    {
                        fs << ", ";
                    }
                }
                fs << "};\n";
                fs << "size_t k_" << image.displayName << EnumToString(k_saveImageFormat) << "Size = " << blob.size() << ";\n\n";
            }
            else
            {
                JAM_ASSERT(0, "Failed to save image: {}", image.displayPath);
                return false;
            }
        }
        return true;
    }

    NODISCARD const TextureRef& GetSceneTexture() const override
    {
        return m_backBufferTexture;
    }

    TextureRef          m_backBufferTexture = {};        // back buffer texture
    std::vector<Image> m_images            = {};        // loaded images
    Image*             m_pSelectedImage    = nullptr;   // currently selected image for preview
};

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

Application* CreateApplication(JAM_MAYBE_UNUSED const CommandLineArguments& _args)
{
    return new MyApplication({ "Shader Compiler", fs::current_path() });
}

JAM_MAIN()