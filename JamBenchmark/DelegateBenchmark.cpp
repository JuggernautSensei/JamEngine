#include "pch.h"
using namespace Jam;

//-----------------------------------------------------------------------------
// 테스트 대상
//-----------------------------------------------------------------------------

static int FreeFunc(int x) { return x + 1; }

struct IBase
{
    virtual int Call(int x) = 0;
    virtual ~IBase()        = default;
};

struct Derived : IBase
{
    int value = 0;
    int Call(int x) override { return x + 1; }
    int CallMethod(int x) { return x + 1; }
};

//-----------------------------------------------------------------------------
// Benchmarks
//-----------------------------------------------------------------------------

// 1. 가상함수
static void BM_VirtualCall(benchmark::State& state)
{
    Derived obj;
    IBase*  p = &obj;
    int     r = 0;
    for (auto _: state)
    {
        r = p->Call(r);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_VirtualCall);

// 2. Delegate — 자유 함수
static void BM_Delegate_FreeFunc(benchmark::State& state)
{
    Delegate<int(int)> d;
    d.Connect<FreeFunc>();
    int r = 0;
    for (auto _: state)
    {
        r = d.Call(r);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_Delegate_FreeFunc);

// 3. Delegate — 멤버 함수
static void BM_Delegate_Method(benchmark::State& state)
{
    Derived            obj;
    Delegate<int(int)> d;
    d.Connect<&Derived::CallMethod>(&obj);
    int r = 0;
    for (auto _: state)
    {
        r = d.Call(r);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_Delegate_Method);

// 4. Delegate — 캡처 없는 람다 (SBO)
static void BM_Delegate_Lambda_NoCapture(benchmark::State& state)
{
    Delegate<int(int)> d;
    d.Connect([](int x) { return x + 1; });
    int r = 0;
    for (auto _: state)
    {
        r = d.Call(r);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_Delegate_Lambda_NoCapture);

// 5. Delegate — 캡처 있는 람다 (SBO 범위 내, 포인터 1개)
static void BM_Delegate_Lambda_Capture_SBO(benchmark::State& state)
{
    Derived            obj;
    Delegate<int(int)> d;
    d.Connect([&obj](int x) { return x + obj.value + 1; });
    int r = 0;
    for (auto _: state)
    {
        r = d.Call(r);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_Delegate_Lambda_Capture_SBO);

// 6. Delegate — 캡처 있는 람다 (SBO 초과, 힙 경유)
static void BM_Delegate_Lambda_Capture_Heap(benchmark::State& state)
{
    Derived            obj1, obj2, obj3;
    Delegate<int(int)> d;
    d.Connect([&obj1, &obj2, &obj3](int x) { return x + obj1.value + obj2.value + obj3.value + 1; });
    int r = 0;
    for (auto _: state)
    {
        r = d.Call(r);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_Delegate_Lambda_Capture_Heap);

// 7. std::function (비교용)
static void BM_StdFunction(benchmark::State& state)
{
    std::function<int(int)> f = FreeFunc;
    int                     r = 0;
    for (auto _: state)
    {
        r = f(r);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_StdFunction);

// 8. 직접 함수 포인터 (베이스라인)
static void BM_FunctionPointer(benchmark::State& state)
{
    int (*fp)(int) = FreeFunc;
    int r          = 0;
    for (auto _: state)
    {
        r = fp(r);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_FunctionPointer);
