#include "pch.h"

#include "SandboxApp.h"
#include "../JamEngine/CommandLineArgumentParser.h"

#include "SimpleScene.h"

class InstancingScene;

void SandboxApp::Init(
    const Int32 _argc,
    char**      _argv)
{
    CommandLineArgumentParser parser { _argc, _argv };
    for (Int32 i = 0; i < parser.GetNumArguments(); ++i)
    {
        const StringView arg = parser.GetArgument(i);
        JAM_LOG_INFO("Command line argument {}: '{}'", i, arg);
    }

    SetVsync(false);
    GetWindowsPlatform().AdjustClientRect(100, 100, 1600, 900);

#define CASE(_m)                          \
    case String::Hashed32(#_m):           \
        sMgr.LoadScene(MakeScoped<_m>()); \
        break;

    SceneManager& sMgr = GetSceneManager();
    if (parser.GetNumArguments() == 2)
    {
        const StringView cmd = parser.GetArgument(1);
        switch (cmd.Hash32())
        {
            CASE(SimpleScene)
                // CASE(InstancingScene)
                // CASE(MultiVertexStreamScene)
                // CASE(TextureMappingScene)
                // CASE(PBRScene)
                // CASE(DeferedScene)
                // CASE(ShadowMappingScene)
                // CASE(DebugNormalScene)

            default:
                JAM_ASSERT(0, "Unknown scene name '{}'.", cmd);
                break;
        }
    }

#undef CASE
}