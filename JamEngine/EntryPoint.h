#pragma once
#include "Application.h"

// define this instead of main function in your application
#ifdef JAM_ENTRY_POINT_MAIN

inline int main(char** argv, const int argc)
{
    using namespace jam;
    const CommandLineArguments args = CommandLineArguments { argv, argc };
    Application::CreateInstance(args);
    Application& app    = Application::GetInstance();
    const int    result = app.Run();
    Application::DestroyInstance();
    return result;
}

#endif