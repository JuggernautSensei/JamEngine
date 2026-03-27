#include "pch.h"
#include <numeric>
#include <algorithm>

// 테스트를 위한 임의의 데이터 타입 (기존 코드의 Jam::Byte 가정)
using Byte = unsigned char;

// --- 1. 초기화 성능 비교 (Zero-initialization) ---

template<size_t Size>
static void BM_RawArray_Fill(benchmark::State& state)
{
    for (auto _: state)
    {
        Byte arr[Size];
        std::fill(arr, arr + Size, 0);   // std::fill 사용
        benchmark::DoNotOptimize(arr);
    }
}

template<size_t Size>
static void BM_StdArray_Fill(benchmark::State& state)
{
    for (auto _: state)
    {
        Array<Byte, Size> arr;
        arr.fill(0);   // Array 전용 fill 사용
        benchmark::DoNotOptimize(arr);
    }
}

// --- 2. 순차 접근 성능 비교 (Sum) ---

template<size_t Size>
static void BM_RawArray_Access(benchmark::State& state)
{
    Byte arr[Size];
    std::fill(arr, arr + Size, 1);   // 1로 채움
    for (auto _: state)
    {
        size_t sum = 0;
        for (size_t i = 0; i < Size; ++i)
        {
            sum += arr[i];
        }
        benchmark::DoNotOptimize(sum);
    }
}

template<size_t Size>
static void BM_StdArray_Access(benchmark::State& state)
{
    Array<Byte, Size> arr;
    arr.fill(1);
    for (auto _: state)
    {
        size_t sum = 0;
        for (size_t i = 0; i < Size; ++i)
        {
            sum += arr[i];   // operator[] 사용
        }
        benchmark::DoNotOptimize(sum);
    }
}

// 벤치마크 등록 (크기: 1024바이트 기준)
BENCHMARK_TEMPLATE(BM_RawArray_Fill, 1024);
BENCHMARK_TEMPLATE(BM_StdArray_Fill, 1024);
BENCHMARK_TEMPLATE(BM_RawArray_Access, 1024);
BENCHMARK_TEMPLATE(BM_StdArray_Access, 1024);
