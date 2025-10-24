#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// 텍스처 상태를 정의하는 enum 클래스
enum class eRWState
{
    None,
    Read,
    Write
};

// 렌더 패스 타입을 정의하는 enum 클래스
enum class ePassType
{
    Graphics,
    Compute,
    Copy
};

// 텍스처 구조체 (실제 프로젝트 구조에 맞게 수정)
struct Texture
{
    std::string name;
    uint32_t    width;
    uint32_t    height;
    eRWState    state;       // 상태 (읽기, 쓰기, 없음)
    uint32_t    boundSlot;   // 바인딩된 슬롯 번호 (-1이면 언바인딩)

    Texture()
        : name("")
        , width(0)
        , height(0)
        , state(eRWState::None)
        , boundSlot(UINT32_MAX)
    {
    }
    Texture(const std::string& n, uint32_t w, uint32_t h)
        : name(n)
        , width(w)
        , height(h)
        , state(eRWState::None)
        , boundSlot(UINT32_MAX)
    {
    }
};

// 샘플러 구조체
struct Sampler
{
    std::string name;
    bool        isAnisotropic;
    bool        compare;
    bool        wrap;
    bool        clamp;
};

// 깊이 및 스텐실 상태 구조체
struct DepthStencil
{
    std::string name;
    uint32_t    width;
    uint32_t    height;
    bool        hasDepth;
    bool        hasStencil;
};

// 레스터라이저 상태 구조체
struct RasterizerState
{
    std::string name;
    bool        cullBackFaces;
    bool        wireframeMode;
};

// 블렌드 상태 구조체 (실제 프로젝트 구조에 맞게 단순화)
struct BlendState
{
    std::string name;
    bool        alphaBlending;
    bool        additiveBlending;
};

// 버퍼 구조체 (실제 프로젝트 구조에 맞게 수정)
struct Buffer
{
    std::string name;
    uint32_t    size;
    bool        isDynamic;
};

// 렌더 그래프 리소스 구조체
struct RenderResource
{
    std::string              name;
    bool                     isTemporary;   // 임시 리소스인지 여부
    bool                     isImported;    // 외부에서 가져온 리소스인지 여부
    std::shared_ptr<Texture> texture;       // 실제 텍스처 데이터

    RenderResource(const std::string& n, bool temp = false, bool imported = false)
        : name(n)
        , isTemporary(temp)
        , isImported(imported)
    {
    }
};

// 렌더 패스 구조체
struct RenderPass
{
    std::string              name;
    ePassType                type;
    std::vector<std::string> readResources;    // 읽기 리소스 목록
    std::vector<std::string> writeResources;   // 쓰기 리소스 목록
    std::function<void()>    executeFunc;      // 패스 실행 함수

    RenderPass(const std::string& n, ePassType t)
        : name(n)
        , type(t)
    {
    }

    void AddRead(const std::string& resourceName)
    {
        readResources.push_back(resourceName);
    }

    void AddWrite(const std::string& resourceName)
    {
        writeResources.push_back(resourceName);
    }

    void SetExecuteFunction(std::function<void()> func)
    {
        executeFunc = func;
    }
};

// 렌더 그래프 클래스
class RenderGraph
{
public:
    RenderGraph()  = default;
    ~RenderGraph() = default;

    // 리소스 생성
    void CreateResource(const std::string& name, uint32_t width, uint32_t height, bool isTemporary = true)
    {
        if (resources.find(name) != resources.end())
        {
            std::cout << "Warning: Resource '" << name << "' already exists.\n";
            return;
        }

        auto resource     = std::make_shared<RenderResource>(name, isTemporary);
        resource->texture = std::make_shared<Texture>(name, width, height);
        resources[name]   = resource;

        std::cout << "Created render resource: " << name << " (" << width << "x" << height << ")\n";
    }

    // 외부 리소스 가져오기 (예: 스왑체인, 깊이 버퍼 등)
    void ImportResource(const std::string& name, std::shared_ptr<Texture> texture)
    {
        if (resources.find(name) != resources.end())
        {
            std::cout << "Warning: Resource '" << name << "' already exists.\n";
            return;
        }

        auto resource     = std::make_shared<RenderResource>(name, false, true);
        resource->texture = texture;
        resources[name]   = resource;

        std::cout << "Imported render resource: " << name << "\n";
    }

    // 렌더 패스 추가
    RenderPass& AddPass(const std::string& name, ePassType type)
    {
        if (passes.find(name) != passes.end())
        {
            std::cout << "Warning: Pass '" << name << "' already exists.\n";
            return *passes[name];
        }

        auto        pass    = std::make_unique<RenderPass>(name, type);
        RenderPass& passRef = *pass;
        passes[name]        = std::move(pass);
        passOrder.push_back(name);

        std::cout << "Added render pass: " << name << "\n";
        return passRef;
    }

    // 렌더 그래프 컴파일 (의존성 검증 및 최적화)
    bool Compile()
    {
        std::cout << "\n=== Compiling RenderGraph ===\n";

        // 1. 리소스 의존성 검증
        if (!ValidateDependencies())
        {
            std::cout << "Error: Resource dependency validation failed.\n";
            return false;
        }

        // 2. 패스 순서 최적화 (단순한 토폴로지 정렬)
        if (!OptimizePassOrder())
        {
            std::cout << "Error: Pass order optimization failed.\n";
            return false;
        }

        // 3. 리소스 생명주기 계산
        CalculateResourceLifetime();

        std::cout << "RenderGraph compilation successful.\n";
        return true;
    }

    // 렌더 그래프 실행
    void Execute()
    {
        std::cout << "\n=== Executing RenderGraph ===\n";

        for (const auto& passName: optimizedPassOrder)
        {
            auto& pass = *passes[passName];
            std::cout << "\nExecuting pass: " << pass.name << "\n";

            // 읽기 리소스 바인딩
            for (const auto& resourceName: pass.readResources)
            {
                BindResourceForRead(resourceName);
            }

            // 쓰기 리소스 바인딩
            for (const auto& resourceName: pass.writeResources)
            {
                BindResourceForWrite(resourceName);
            }

            // 패스 실행
            if (pass.executeFunc)
            {
                pass.executeFunc();
            }
            else
            {
                std::cout << "  [No execute function defined for this pass]\n";
            }

            // 리소스 언바인딩
            UnbindResources(pass);
        }

        std::cout << "\nRenderGraph execution completed.\n";
    }

    // 렌더 그래프 정보 출력
    void PrintInfo()
    {
        std::cout << "\n=== RenderGraph Information ===\n";

        std::cout << "Resources (" << resources.size() << "):\n";
        for (const auto& [name, resource]: resources)
        {
            std::cout << "  " << name;
            if (resource->isTemporary) std::cout << " (Temporary)";
            if (resource->isImported) std::cout << " (Imported)";
            if (resource->texture)
            {
                std::cout << " [" << resource->texture->width << "x" << resource->texture->height << "]";
            }
            std::cout << "\n";
        }

        std::cout << "\nPasses (" << passes.size() << "):\n";
        for (const auto& passName: passOrder)
        {
            const auto& pass = *passes.at(passName);
            std::cout << "  " << pass.name << " (" << GetPassTypeName(pass.type) << ")\n";

            if (!pass.readResources.empty())
            {
                std::cout << "    Reads: ";
                for (size_t i = 0; i < pass.readResources.size(); ++i)
                {
                    if (i > 0) std::cout << ", ";
                    std::cout << pass.readResources[i];
                }
                std::cout << "\n";
            }

            if (!pass.writeResources.empty())
            {
                std::cout << "    Writes: ";
                for (size_t i = 0; i < pass.writeResources.size(); ++i)
                {
                    if (i > 0) std::cout << ", ";
                    std::cout << pass.writeResources[i];
                }
                std::cout << "\n";
            }
        }

        if (!optimizedPassOrder.empty() && optimizedPassOrder != passOrder)
        {
            std::cout << "\nOptimized pass order: ";
            for (size_t i = 0; i < optimizedPassOrder.size(); ++i)
            {
                if (i > 0) std::cout << " -> ";
                std::cout << optimizedPassOrder[i];
            }
            std::cout << "\n";
        }

        std::cout << "===============================\n";
    }

    // 렌더 그래프 초기화
    void Reset()
    {
        resources.clear();
        passes.clear();
        passOrder.clear();
        optimizedPassOrder.clear();
        std::cout << "RenderGraph reset.\n";
    }

private:
    std::unordered_map<std::string, std::shared_ptr<RenderResource>> resources;
    std::unordered_map<std::string, std::unique_ptr<RenderPass>>     passes;
    std::vector<std::string>                                         passOrder;            // 원본 패스 순서
    std::vector<std::string>                                         optimizedPassOrder;   // 최적화된 패스 순서

    // 패스 타입 이름 반환
    std::string GetPassTypeName(ePassType type)
    {
        switch (type)
        {
            case ePassType::Graphics: return "Graphics";
            case ePassType::Compute: return "Compute";
            case ePassType::Copy: return "Copy";
            default: return "Unknown";
        }
    }

    // 리소스 의존성 검증
    bool ValidateDependencies()
    {
        std::cout << "Validating resource dependencies...\n";

        for (const auto& [passName, pass]: passes)
        {
            // 읽기 리소스 존재 확인
            for (const auto& resourceName: pass->readResources)
            {
                if (resources.find(resourceName) == resources.end())
                {
                    std::cout << "Error: Pass '" << passName << "' tries to read non-existent resource '" << resourceName << "'\n";
                    return false;
                }
            }

            // 쓰기 리소스 존재 확인
            for (const auto& resourceName: pass->writeResources)
            {
                if (resources.find(resourceName) == resources.end())
                {
                    std::cout << "Error: Pass '" << passName << "' tries to write to non-existent resource '" << resourceName << "'\n";
                    return false;
                }
            }
        }

        return true;
    }

    // 패스 순서 최적화 (단순한 버전)
    bool OptimizePassOrder()
    {
        std::cout << "Optimizing pass order...\n";

        // 현재는 단순히 원본 순서를 유지
        // 실제로는 더 복잡한 토폴로지 정렬이 필요
        optimizedPassOrder = passOrder;

        return true;
    }

    // 리소스 생명주기 계산
    void CalculateResourceLifetime()
    {
        std::cout << "Calculating resource lifetimes...\n";

        // 각 리소스가 어느 패스에서 처음 사용되고 마지막으로 사용되는지 계산
        // 현재는 단순한 버전으로 구현
        for (const auto& [resourceName, resource]: resources)
        {
            int firstUse = -1;
            int lastUse  = -1;

            for (int i = 0; i < optimizedPassOrder.size(); ++i)
            {
                const auto& pass = *passes[optimizedPassOrder[i]];

                bool usesResource = false;
                for (const auto& readRes: pass.readResources)
                {
                    if (readRes == resourceName)
                    {
                        usesResource = true;
                        break;
                    }
                }

                if (!usesResource)
                {
                    for (const auto& writeRes: pass.writeResources)
                    {
                        if (writeRes == resourceName)
                        {
                            usesResource = true;
                            break;
                        }
                    }
                }

                if (usesResource)
                {
                    if (firstUse == -1) firstUse = i;
                    lastUse = i;
                }
            }

            if (firstUse != -1 && resource->isTemporary)
            {
                std::cout << "  Resource '" << resourceName << "' lifetime: pass " << firstUse << " to " << lastUse << "\n";
            }
        }
    }

    // 읽기용 리소스 바인딩
    void BindResourceForRead(const std::string& resourceName)
    {
        auto it = resources.find(resourceName);
        if (it != resources.end() && it->second->texture)
        {
            auto& texture = *it->second->texture;
            if (texture.state == eRWState::Write)
            {
                std::cout << "  Warning: Resource '" << resourceName << "' is in write state, transitioning to read.\n";
            }
            texture.state = eRWState::Read;
            std::cout << "  Bound '" << resourceName << "' for reading.\n";
        }
    }

    // 쓰기용 리소스 바인딩
    void BindResourceForWrite(const std::string& resourceName)
    {
        auto it = resources.find(resourceName);
        if (it != resources.end() && it->second->texture)
        {
            auto& texture = *it->second->texture;
            texture.state = eRWState::Write;
            std::cout << "  Bound '" << resourceName << "' for writing.\n";
        }
    }

    // 리소스 언바인딩
    void UnbindResources(const RenderPass& pass)
    {
        for (const auto& resourceName: pass.readResources)
        {
            auto it = resources.find(resourceName);
            if (it != resources.end() && it->second->texture)
            {
                it->second->texture->state = eRWState::None;
            }
        }

        for (const auto& resourceName: pass.writeResources)
        {
            auto it = resources.find(resourceName);
            if (it != resources.end() && it->second->texture)
            {
                it->second->texture->state = eRWState::None;
            }
        }

        std::cout << "  Unbound resources for pass: " << pass.name << "\n";
    }
};

// 그래픽 API를 나타내는 클래스 (기존 코드 유지)
class GraphicsAPI
{
public:
    // 텍스처 생성 함수
    Texture CreateTexture(const std::string& name, uint32_t width, uint32_t height)
    {
        auto it = std::find_if(textures.begin(), textures.end(), [&name](const Texture& tex)
                               { return tex.name == name; });

        if (it != textures.end())
        {
            std::cout << "Warning: Texture with name '" << name << "' already exists.\n";
            return *it;
        }

        Texture newTexture(name, width, height);
        textures.push_back(newTexture);
        std::cout << "Created texture: " << name << " (" << width << "x" << height << ")\n";
        return newTexture;
    }

    // 기존 함수들 유지...
    void BindTexture(uint32_t slot, const std::string& name) { /* 기존 구현 */ }
    void UnbindTexture(const std::string& name) { /* 기존 구현 */ }

private:
    std::vector<Texture>         textures;
    std::vector<Sampler>         samplers;
    std::vector<DepthStencil>    depthStencils;
    std::vector<RasterizerState> rasterizerStates;
    std::vector<BlendState>      blendStates;
    std::vector<Buffer>          buffers;
};

int main()
{
    std::cout << "=== RenderGraph Demo ===\n\n";

    RenderGraph renderGraph;
    GraphicsAPI graphicsAPI;

    // 1. 리소스 생성
    std::cout << "--- Creating Resources ---\n";
    renderGraph.CreateResource("GBuffer_Albedo", 1920, 1080);
    renderGraph.CreateResource("GBuffer_Normal", 1920, 1080);
    renderGraph.CreateResource("GBuffer_Material", 1920, 1080);
    renderGraph.CreateResource("DepthBuffer", 1920, 1080);
    renderGraph.CreateResource("LightingResult", 1920, 1080);
    renderGraph.CreateResource("FinalImage", 1920, 1080);

    // 외부 리소스 가져오기 (스왑체인)
    auto backbuffer = std::make_shared<Texture>("Backbuffer", 1920, 1080);
    renderGraph.ImportResource("Backbuffer", backbuffer);

    // 2. 렌더 패스 정의
    std::cout << "\n--- Defining Render Passes ---\n";

    // G-Buffer 패스
    auto& gbufferPass = renderGraph.AddPass("GBufferPass", ePassType::Graphics);
    gbufferPass.AddWrite("GBuffer_Albedo");
    gbufferPass.AddWrite("GBuffer_Normal");
    gbufferPass.AddWrite("GBuffer_Material");
    gbufferPass.AddWrite("DepthBuffer");
    gbufferPass.SetExecuteFunction([]()
                                   {
        std::cout << "    -> Rendering geometry to G-Buffer\n";
        std::cout << "    -> Writing albedo, normal, material properties\n"; });

    // 라이팅 패스
    auto& lightingPass = renderGraph.AddPass("LightingPass", ePassType::Graphics);
    lightingPass.AddRead("GBuffer_Albedo");
    lightingPass.AddRead("GBuffer_Normal");
    lightingPass.AddRead("GBuffer_Material");
    lightingPass.AddRead("DepthBuffer");
    lightingPass.AddWrite("LightingResult");
    lightingPass.SetExecuteFunction([]()
                                    {
        std::cout << "    -> Reading G-Buffer data\n";
        std::cout << "    -> Computing deferred lighting\n";
        std::cout << "    -> Writing lighting result\n"; });

    // 포스트 프로세싱 패스
    auto& postprocessPass = renderGraph.AddPass("PostProcessPass", ePassType::Graphics);
    postprocessPass.AddRead("LightingResult");
    postprocessPass.AddWrite("FinalImage");
    postprocessPass.SetExecuteFunction([]()
                                       {
        std::cout << "    -> Applying tone mapping\n";
        std::cout << "    -> Applying color grading\n"; });

    // 프레젠트 패스
    auto& presentPass = renderGraph.AddPass("PresentPass", ePassType::Copy);
    presentPass.AddRead("FinalImage");
    presentPass.AddWrite("Backbuffer");
    presentPass.SetExecuteFunction([]()
                                   {
        std::cout << "    -> Copying final image to backbuffer\n";
        std::cout << "    -> Presenting to screen\n"; });

    // 3. 렌더 그래프 정보 출력
    renderGraph.PrintInfo();

    // 4. 렌더 그래프 컴파일
    if (renderGraph.Compile())
    {
        // 5. 렌더 그래프 실행
        renderGraph.Execute();
    }

    // 6. 리셋 테스트
    std::cout << "\n--- Reset Test ---\n";
    renderGraph.Reset();

    return 0;
}