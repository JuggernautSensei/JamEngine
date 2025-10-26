#include "pch.h"

#include "ExampleApp.h"

Application* CreateApplication(const CommandLineArguments& _args)
{
    for (Int32 i = 0; i < _args.argCount; ++i)
        Log::Info("Arg[{}]: {}", i, _args.GetArgument(i));

    ApplicationCreateInfo info;
    info.applicationName  = "Example Application";
    info.workingDirectory = fs::current_path();
    info.bEditor          = false;
    return new ExampleApp(info);
}

JAM_MAIN();