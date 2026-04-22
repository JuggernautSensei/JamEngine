#include "pch.h"

int main(int _argc, char** _argv)
{
    JAM_ASSERT(_argc >= 0 && _argv != nullptr, "Invalid arguments");

    benchmark::MaybeReenterWithoutASLR(_argc, _argv);

    char  arg0_default[] = "benchmark";
    char* args_default   = arg0_default;
    if (!_argv)
    {
        _argc = 1;
        _argv = &args_default;
    }

    ::benchmark::Initialize(&_argc, _argv);
    if (::benchmark::ReportUnrecognizedArguments(_argc, _argv))
    {
        JAM_LOG_ERROR("Failed to parse benchmark arguments.");
        return 1;
    }

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    return 0;
}
