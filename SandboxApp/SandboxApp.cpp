#include "pch.h"

#include "SandboxApp.h"

#include "SimpleScene.h"
#include "../JamEngine/CommandLineArgumentParser.h"

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

    SceneManager& sMgr         = GetSceneManager();
    bool          bSceneLoaded = false;
    if (parser.GetNumArguments() == 2)
    {
        const StringView cmd = parser.GetArgument(1);
        bSceneLoaded         = true;
        switch (cmd.Hash32())
        {
            case String::Hashed32("SimpleScene"):
                sMgr.LoadScene(MakeScoped<SimpleScene>());
                break;

            default:
                JAM_LOG_WARN("Unknown scene specified in command line argument: '{}'", cmd);
                bSceneLoaded = false;
                break;
        }
    }

    if (!bSceneLoaded)
    {
        sMgr.LoadScene(MakeScoped<SimpleScene>());   // fallback
    }
}