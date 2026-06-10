#include "pch.h"

#include "SandboxApp.h"

#include "DebugNormalScene.h"
#include "DeferredScene.h"
#include "../JamEngine/CommandLineParser.h"

#include "SimpleScene.h"
#include "InstancingScene.h"
#include "MultiVertexStreamScene.h"
#include "PBRScene.h"
#include "ImageFilterManager.h"
#include "ShadowMappingScene.h"
#include "TextureMappingScene.h"
#include "ComputeScene.h"

#define SWITCH_CASE(_m)                 \
    case HashOf(#_m):                   \
        sm.LoadScene(MakeScoped<_m>()); \
        bLoadSucceeded = true;          \
        break;

void SandboxApp::Init(
    const Int32 _argc,
    char**      _argv)
{
    ImageFilterManager::CreateInstance();

    SetVsync(true);
    WindowsPlatform& win = GetWindowsPlatform();
    win.AdjustClientRect(100, 100, 1600, 900);
    SceneManager& sm = GetSceneManager();

    CommandLineParser    parser { _argc, _argv };
    Optional<StringView> scene          = parser.GetString("--scene");
    bool                 bLoadSucceeded = false;

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
            SWITCH_CASE(ComputeScene)
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
}

void SandboxApp::Shutdown()
{
    ImageFilterManager::DestroyInstance();
}

int main(
    const int _argc,
    char**    _argv)
{
    CommandLineParser    parser { _argc, _argv };
    Flags<eAppInitFlags> flags = kDefaultAppInitFlags;

    if (parser.HasFlag("--editor"))
    {
        flags |= eAppInitFlags::Subsystem_Editor;
    }

    if (parser.HasFlag("--allow-tearing"))
    {
        flags.Enable(eAppInitFlags::Graphics_AllowTearing);
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
        flags.Enable(eAppInitFlags::Graphics_GraphicsDebugger);
    }

    SandboxApp app { "SandboxApp", flags };
    return app.Run(_argc, _argv);
}