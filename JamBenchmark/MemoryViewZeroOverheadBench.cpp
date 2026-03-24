#include "pch.h"
#include <vector>
#include <cstdint>

void DummyMix(const Jam::Byte* data, Jam::SizeT size)
{
    benchmark::DoNotOptimize(data);
    benchmark::DoNotOptimize(size);
}

void DummyMix(Jam::MemoryView view)
{
    benchmark::DoNotOptimize(view.GetPtr());
    benchmark::DoNotOptimize(view.GetByteWidth());
}

// 1. Raw 포인터 오버헤드 측정
static void BM_RawPointer(benchmark::State& state)
{
    uint64_t data = 0x123456789ABCDEF0;
    for (auto _: state)
    {
        DummyMix(reinterpret_cast<const Jam::Byte*>(&data), sizeof(data));
    }
}
BENCHMARK(BM_RawPointer);

// 2. MemoryView 객체 생성 및 전달 오버헤드 측정
static void BM_MemoryView(benchmark::State& state)
{
    uint64_t data = 0x123456789ABCDEF0;
    for (auto _: state)
    {
        Jam::MemoryView view(reinterpret_cast<const Jam::Byte*>(&data), sizeof(data));
        DummyMix(view);
    }
}
BENCHMARK(BM_MemoryView);

// 3. AsMemoryView 헬퍼 함수 오버헤드 측정
static void BM_AsMemoryView(benchmark::State& state)
{
    uint64_t data = 0x123456789ABCDEF0;
    for (auto _: state)
    {
        DummyMix(Jam::AsMemoryView(data));
    }
}
BENCHMARK(BM_AsMemoryView);
