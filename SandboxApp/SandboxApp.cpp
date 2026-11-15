#include "pch.h"

#include "SandboxApp.h"

#include "DebugNormalScene.h"
#include "DeferredScene.h"
#include "../JamEngine/CommandLineParser.h"
#include "../JamEngine/Image.h"
#include "../JamEngine/FileSystem.h"
#include <charconv>

#include "SimpleScene.h"
#include "InstancingScene.h"
#include "MultiVertexStreamScene.h"
#include "PBRScene.h"
#include "ImageFilterManager.h"
#include "ShadowMappingScene.h"
#include "StencilScene.h"
#include "CullingScene.h"
#include "TerrainScene.h"
#include "SSAOScene.h"
#include "SSRScene.h"
#include "ParticleScene.h"
#include "MousePickingScene.h"
#include "TextureMappingScene.h"
#include "ComputeScene.h"
#include "SkinningScene.h"

#define SWITCH_CASE(_m)                        \
    case HashOf(#_m):                          \
        sm.LoadScene(MakeScoped<_m>(), false); \
        bLoadSucceeded = true;                 \
        break;

void SandboxApp::Init(
    const Int32 _argc,
    char**      _argv)
{
    ImageFilterManager::CreateInstance();

    Graphics& gfx = GetGraphics();
    gfx.SetVSync(true);

    WindowsPlatform& win = GetWindowsPlatform();
    win.AdjustClientRect(100, 100, 1600, 900);
    SceneManager& sm = GetSceneManager();

    CommandLineParser    parser { _argc, _argv };
    Optional<StringView> scene          = parser.GetStringOr("--scene", "SimpleScene");
    bool                 bLoadSucceeded = false;
    m_remainingFrames                   = parser.GetIntOr("--frames", 0);
    m_screenshotPath                    = parser.GetStringOr("--screenshot", "");

    if (scene)
    {
        switch (scene->Hash())
        {
            SWITCH_CASE(SimpleScene)
            SWITCH_CASE(InstancingScene)
            SWITCH_CASE(MultiVertexStreamScene)
            SWITCH_CASE(TextureMappingScene)
            SWITCH_CASE(PBRScene)
            SWITCH_CASE(DeferredScene)
            SWITCH_CASE(DebugNormalScene)
            SWITCH_CASE(ShadowMappingScene)
            SWITCH_CASE(StencilScene)
            SWITCH_CASE(CullingScene)
            SWITCH_CASE(TerrainScene)
            SWITCH_CASE(SSAOScene)
            SWITCH_CASE(SSRScene)
            SWITCH_CASE(ParticleScene)
            SWITCH_CASE(MousePickingScene)
            SWITCH_CASE(ComputeScene)
            SWITCH_CASE(SkinningScene)
            default:
                break;
        }
    }

    if (!bLoadSucceeded)
    {
        JAM_LOG_ERROR("Failed to load scene. Command line argument '--scene' is either not provided or invalid");
        Application& app = GetApp();
        app.Quit();
    }
    else if (*scene == "ComputeScene" && parser.HasFlag("--demo"))
    {
        static_cast<ComputeScene*>(sm.GetActiveScene())->RunDemo(parser.GetIntOr("--demo", 1) - 1);
    }
    else if (*scene == "ParticleScene")
    {
        ParticleScene& particles = *static_cast<ParticleScene*>(sm.GetActiveScene());
        particles.RunDemo(parser.GetIntOr("--demo", 1) - 1);
        if (parser.HasFlag("--particles"))
            particles.SetParticleCount(parser.GetIntOr("--particles", 12000));
    }
    else if (*scene == "MousePickingScene")
    {
        MousePickingScene& picking = *static_cast<MousePickingScene*>(sm.GetActiveScene());
        picking.SetShowIds(parser.HasFlag("--show-ids"));
        if (parser.HasFlag("--pick-x"))
            picking.RequestPick({ parser.GetIntOr("--pick-x", 0), parser.GetIntOr("--pick-y", 0) });
    }
}

void SandboxApp::Shutdown()
{
    ImageFilterManager::DestroyInstance();
}

void SandboxApp::Update(
    const float _dt)
{
    Application::Update(_dt);
    if (m_remainingFrames > 0 && --m_remainingFrames == 0)
    {
        Quit();
    }
}

void SandboxApp::Render()
{
    Application::Render();
    if (m_remainingFrames == 0 && !m_screenshotPath.IsEmpty())
    {
        Graphics& gfx = GetGraphics();
        if (const Error err = gfx.Capture(gfx.GetBackBuffer().texh).ExportToFile(m_screenshotPath, eImageFormat::PNG))
        {
            JAM_CRASH("Failed to save screenshot: {}", err);
        }
        m_screenshotPath.Clear();
    }
}

int main(
    const int _argc,
    char**    _argv)
{
    CommandLineParser    parser { _argc, _argv };
    constexpr StringView kScenes[] = {
        "SimpleScene", "InstancingScene", "MultiVertexStreamScene", "TextureMappingScene", "PBRScene", "DeferredScene", "DebugNormalScene", "ShadowMappingScene", "StencilScene", "CullingScene", "TerrainScene", "SSAOScene", "SSRScene", "ParticleScene", "MousePickingScene", "ComputeScene", "SkinningScene"
    };
    if (parser.HasFlag("--help") || parser.HasFlag("--list-scenes"))
    {
        ::printf("SandboxApp --scene NAME [--frames N] [--screenshot PATH] [--demo 1..17]\n");
        ::printf("ParticleScene: --demo 1 (snow) or 2 (confetti), --particles 0..100000\n");
        ::printf("MousePickingScene: --pick-x X --pick-y Y (client pixels), --show-ids\n");
        for (const StringView name: kScenes)
        {
            ::printf("  %s\n", name.GetPtr());
        }
        return 0;
    }

    const StringView scene       = parser.GetStringOr("--scene", "SimpleScene");
    bool             bValidScene = false;
    for (const StringView name: kScenes)
    {
        bValidScene |= scene == name;
    }
    if (!bValidScene || (parser.HasFlag("--scene") && !parser.GetString("--scene")))
    {
        ::fprintf(stderr, "Unknown scene. Use --list-scenes to list examples.\n");
        return 1;
    }
    for (const StringView option: { StringView { "--frames" }, StringView { "--demo" }, StringView { "--particles" }, StringView { "--pick-x" }, StringView { "--pick-y" } })
    {
        if (parser.HasFlag(option))
        {
            const StringView value           = parser.GetStringOr(option, "");
            Int32            parsed          = 0;
            const auto       result          = std::from_chars(value.GetPtr(), value.GetTerminate(), parsed);
            const Int32      maxDemo         = scene == "ComputeScene" ? 17 : scene == "ParticleScene" ? 2
                                                                                                       : 0;
            const bool       bPickCoordinate = option == "--pick-x" || option == "--pick-y";
            bool             bInvalidValue   = parsed <= 0;
            if (bPickCoordinate)
                bInvalidValue = parsed < 0 || scene != "MousePickingScene";
            else if (option == "--particles")
                bInvalidValue = parsed < 0 || parsed > ParticleScene::kCapacity || scene != "ParticleScene";
            if (result.ec != std::errc {} || result.ptr != value.GetTerminate()
                || bInvalidValue
                || (option == "--demo" && parsed > maxDemo))
            {
                ::fprintf(stderr, "Invalid %s value.\n", option.GetPtr());
                return 1;
            }
        }
    }
    if (parser.HasFlag("--pick-x") != parser.HasFlag("--pick-y"))
    {
        ::fprintf(stderr, "--pick-x and --pick-y must be supplied together.\n");
        return 1;
    }
    if (parser.HasFlag("--show-ids") && scene != "MousePickingScene")
    {
        ::fprintf(stderr, "--show-ids requires MousePickingScene.\n");
        return 1;
    }
    if (parser.HasFlag("--screenshot") && (!parser.GetString("--screenshot") || !parser.HasFlag("--frames")))
    {
        ::fprintf(stderr, "--screenshot requires a path and --frames.\n");
        return 1;
    }
    if (parser.HasFlag("--editor"))
    {
        ::fprintf(stderr, "The retired editor is unavailable. Launch a sandbox scene directly.\n");
        return 1;
    }

    const String executable = FS::Executable();
    const String resources  = String::Joined(executable.GetDirectory(), "Resources/");
    if (FS::IsDirectory(resources))
    {
        const WString directory = WString::Encoded(executable.GetDirectory());
        if (!::SetCurrentDirectoryW(directory.GetPtr()))
        {
            return 1;
        }
    }
    Flags<eAppInitFlags> flags = kDefaultAppInitFlags;

    if (parser.HasFlag("--triple-buffering"))
    {
        flags.Enable(eAppInitFlags::Graphics_TripleBuffering);
    }
    else if (parser.HasFlag("--quadruple-buffering"))
    {
        flags.Enable(eAppInitFlags::Graphics_QuadrupleBuffering);
    }

    if (parser.HasFlag("--hdr10"))
    {
        flags.Enable(eAppInitFlags::Graphics_BackBufferHDR10);
    }
    else if (parser.HasFlag("--hdr16"))
    {
        flags.Enable(eAppInitFlags::Graphics_BackBufferHDR16);
    }

    if (parser.HasFlag("--msaa4"))
    {
        flags.Enable(eAppInitFlags::Graphics_BackBufferMSAAx4);
    }
    else if (parser.HasFlag("--msaa8"))
    {
        flags.Enable(eAppInitFlags::Graphics_BackBufferMSAAx8);
    }
    else if (parser.HasFlag("--msaa16"))
    {
        flags.Enable(eAppInitFlags::Graphics_BackBufferMSAAx16);
    }

    if (parser.HasFlag("--graphics-debugger"))
    {
        flags.Enable(eAppInitFlags::Graphics_ConnectDebugger);
    }

    SandboxApp app { "SandboxApp", flags };
    return app.Run(_argc, _argv);
}
