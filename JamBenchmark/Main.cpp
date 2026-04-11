#include "pch.h"
void FreeFunction(int& x)
{
    x++;
}

class Base
{
public:
    virtual ~Base()                  = default;
    virtual void VirtualCall(int& x) = 0;
};

class Derived : public Base
{
public:
    void VirtualCall(int& x) override
    {
        x++;
    }
};

// --- 벤치마크 케이스 ---

// 1. 일반 함수 호출
static void BM_DirectCall(benchmark::State& state)
{
    int x = 0;
    for (auto _: state)
    {
        FreeFunction(x);
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(BM_DirectCall);

// 2. 가상 함수 호출
static void BM_VirtualCall(benchmark::State& state)
{
    Base* obj = new Derived();
    int   x   = 0;
    for (auto _: state)
    {
        obj->VirtualCall(x);
        benchmark::DoNotOptimize(x);
    }
    delete obj;
}
BENCHMARK(BM_VirtualCall);

// 3. 함수 포인터 호출
static void BM_FunctionPointerCall(benchmark::State& state)
{
    void (*funcPtr)(int&) = FreeFunction;
    int x                 = 0;
    for (auto _: state)
    {
        funcPtr(x);
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(BM_FunctionPointerCall);

// 4. std::function 호출
static void BM_StdFunctionCall(benchmark::State& state)
{
    std::function<void(int&)> func = FreeFunction;
    int                       x    = 0;
    for (auto _: state)
    {
        func(x);
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(BM_StdFunctionCall);

// 5. Jam::Delegate 호출 (추가됨)
static void BM_JamDelegateCall(benchmark::State& state)
{
    auto del = Jam::Bind<FreeFunction>();
    int  x   = 0;
    for (auto _: state)
    {
        del(x);
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(BM_JamDelegateCall);
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
