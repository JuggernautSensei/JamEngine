#include "pch.h"

#include "EntryPoint.h"

#include "Application.h"

namespace jam::detail
{

int EntryPoint(const int argc, char* argv[])
{
    using namespace jam;
    const CommandLineArguments args = CommandLineArguments { argc, argv };
    Application::Create(args);
    Application& app    = Application::GetInstance();
    const int    result = app.Run();
    Application::Destroy();
    return result;
}

}   // namespace jam::detail
