#include "pch.h"

#include "ExampleApp.h"

Application* CreateApplication(const CommandLineArguments& _args)
{
    for (Int32 i = 0; i < _args.numArgs; ++i)
        Log::Info("arg[{}]: {}", i, _args.GetArgument(i));

    ApplicationCreateInfo info;
    info.applicationName  = "Example Application";
    info.workingDirectory = fs::current_path();
    info.bEnableEditor    = false;

    if (_args.numArgs > 1)
    {
        const char* arg = _args.GetArgument(1);
        if (std::strcmp(arg, "-editor") == 0)
        {
            info.bEnableEditor = true;
            Log::Info("jam engine editor enabled");
        }
    }

    return new ExampleApp(info);
}

JAM_MAIN();