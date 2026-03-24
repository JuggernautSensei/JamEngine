#include "pch.h"

class TestApp : public Application
{
public:
    TestApp(
        const StringView                _appName,
        const Flags<eAppCoreFlags> _subSystems)
        : Application(_appName, _subSystems)
    {
    }

    void Init(Int32 _argc, char** _argv) override
    {
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
            JAM_LOG_ERROR("Failed to parse benchmark arguments. Unrecognized arguments:");
            for (Int32 i = 1; i < _argc; ++i)
            {
                JAM_LOG_ERROR("  {}", _argv[i]);
            }
            JAM_ASSERT(false, "Failed to parse benchmark arguments. Check the log for details.");
        }
    }

    void Update(float _dt) override
    {
        Quit();
    }

    void Shutdown() override
    {
        ::benchmark::RunSpecifiedBenchmarks();
        ::benchmark::Shutdown();
    }
};

JAM_MAIN(TestApp, "JamAppTest", k_runtimeDefaltFlags);
