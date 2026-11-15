/**
 * BenchMatrix.cpp
 * Google Benchmark — Jam::Mtx4 SRT 연산 비교
 *
 * 빌드 예시 (CMake):
 *   find_package(benchmark REQUIRED)
 *   add_executable(BenchMatrix BenchMatrix.cpp)
 *   target_link_libraries(BenchMatrix PRIVATE benchmark::benchmark Jam)
 *
 * 직접 컴파일 예시 (g++ / clang++):
 *   g++ -O2 -std=c++17 BenchMatrix.cpp -lbenchmark -lpthread -o BenchMatrix
 *   ./BenchMatrix
 *
 * 벤치마크 목록
 *   [1] BM_CreateSRT_Fused       — CreateSRT(scale, quat, translation) 단일 호출
 *   [1] BM_CreateSRT_Separate    — CreateScale * FromQuaternion * CreateTranslation 분리 호출
 *   [2] BM_InvSRT_Inverse        — CreateSRT → .Inverse()
 *   [2] BM_InvSRT_CreateInvSRT   — CreateInvSRT(scale, quat, translation) 단일 호출
 */

#include "pch.h"

// ─────────────────────────────────────────────
//  컴파일러 최적화로 결과가 제거되는 것을 막기 위한
//  DoNotOptimize 래퍼 (benchmark 라이브러리 내장)
// ─────────────────────────────────────────────

// 테스트에 사용할 고정 입력값
// (매 반복마다 같은 값을 쓰므로 분기 예측 / 캐시 효과가 공평하게 적용됨)
static const Jam::Vec3 kScale { 2.f, 3.f, 0.5f };
static const Jam::Vec3 kTranslation { 10.f, -5.f, 7.f };

// 단위 쿼터니언 (45° Y축 회전)
static const Jam::Quat kQuat = [] {
    // Quat(w, x, y, z) — 헤더 확인 후 생성자에 맞춰 조정하세요
    constexpr float half = 0.392699f;   // pi/8
    return Jam::Quat(
        /* w */ 0.9238795f,
        /* x */ 0.f,
        /* y */ 0.3826834f,
        /* z */ 0.f);
}();

// ═════════════════════════════════════════════
//  벤치마크 1 : CreateSRT  vs  S * R * T 분리
// ═════════════════════════════════════════════

/**
 * [1-A] 융합 버전
 *   내부적으로 쿼터니언 성분을 한 번 전개하여
 *   S·R 곱과 T 배치를 단일 행렬 생성으로 처리한다.
 */
static void BM_CreateSRT_Fused(benchmark::State& state)
{
    for (auto _: state)
    {
        Jam::Mtx4 result = Jam::Mtx4::CreateSRT(kScale, kQuat, kTranslation);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_CreateSRT_Fused);

/**
 * [1-B] 분리 버전
 *   CreateScale()      → 4×4 대각 행렬 1개
 *   FromQuaternion()   → 4×4 회전 행렬 1개
 *   CreateTranslation()→ 4×4 이동 행렬 1개
 *   operator*(*)       → 행렬 곱 2회 (S*R, (S*R)*T)
 *
 *   행렬 곱 2회 + 행렬 생성 3회의 오버헤드가 측정된다.
 */
static void BM_CreateSRT_Separate(benchmark::State& state)
{
    for (auto _: state)
    {
        const Jam::Mtx4 S      = Jam::Mtx4::CreateScale(kScale);
        const Jam::Mtx4 R      = Jam::Mtx4::FromQuaternion(kQuat);
        const Jam::Mtx4 T      = Jam::Mtx4::MakeTranslation(kTranslation);
        const Jam::Mtx4 result = S * R * T;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_CreateSRT_Separate);

// ═════════════════════════════════════════════
//  벤치마크 2 : CreateSRT→Inverse  vs  CreateInvSRT
// ═════════════════════════════════════════════

/**
 * [2-A] 일반 역행렬 버전
 *   SRT 행렬을 먼저 만든 뒤, 범용 Inverse()를 호출한다.
 *   Inverse()는 4×4 행렬식 + 여인수 행렬을 완전히 계산하므로
 *   약 100개 이상의 부동소수점 연산이 필요하다.
 */
static void BM_InvSRT_Inverse(benchmark::State& state)
{
    for (auto _: state)
    {
        const Jam::Mtx4 srt    = Jam::Mtx4::CreateSRT(kScale, kQuat, kTranslation);
        const Jam::Mtx4 result = srt.Inverse();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_InvSRT_Inverse);

/**
 * [2-B] 특수화된 역행렬 버전
 *   SRT 구조를 수학적으로 이용한다:
 *     inv(SRT) = inv(T) * inv(R) * inv(S)
 *             = T(-t) * R^T * S(1/s)
 *   이 성질을 이용해 행렬 생성 1회만으로 역행렬을 직접 계산한다.
 *   Inverse()에 비해 나눗셈 3회 + 쿼터니언 전개만 수행하므로
 *   대폭 빠를 것으로 예상된다.
 */
static void BM_InvSRT_CreateInvSRT(benchmark::State& state)
{
    for (auto _: state)
    {
        const Jam::Mtx4 result = Jam::Mtx4::CreateInvSRT(kScale, kQuat, kTranslation);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_InvSRT_CreateInvSRT);
