#include "pch.h"
#include <DirectXTK/SimpleMath.h>
#include <DirectXCollision.h>

using namespace Jam;
using namespace DirectX::SimpleMath;

const Matrix Matrix::Identity = Matrix {
    { 1.f, 0.f, 0.f, 0.f },
    { 0.f, 1.f, 0.f, 0.f },
    { 0.f, 0.f, 1.f, 0.f },
    { 0.f, 0.f, 0.f, 1.f }
};

// ============================================================================
// Vector2 Benchmarks
// ============================================================================

static void BM_JamVec2Add(benchmark::State& state)
{
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a + b);
    }
}
BENCHMARK(BM_JamVec2Add)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec2Add(benchmark::State& state)
{
    Vector2 a(1.0f, 2.0f);
    Vector2 b(3.0f, 4.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a + b);
    }
}
BENCHMARK(BM_DirectXVec2Add)->Unit(benchmark::kNanosecond);

static void BM_JamVec2Dot(benchmark::State& state)
{
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Dot(b));
    }
}
BENCHMARK(BM_JamVec2Dot)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec2Dot(benchmark::State& state)
{
    Vector2 a(1.0f, 2.0f);
    Vector2 b(3.0f, 4.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Dot(b));
    }
}
BENCHMARK(BM_DirectXVec2Dot)->Unit(benchmark::kNanosecond);

static void BM_JamVec2Normalize(benchmark::State& state)
{
    Vec2 a(3.0f, 4.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Normalized());
    }
}
BENCHMARK(BM_JamVec2Normalize)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec2Normalize(benchmark::State& state)
{
    Vector2 a(3.0f, 4.0f);
    for (auto _: state)
    {
        a.Normalize();
        benchmark::DoNotOptimize(a);
    }
}
BENCHMARK(BM_DirectXVec2Normalize)->Unit(benchmark::kNanosecond);

// ============================================================================
// Vector3 Benchmarks
// ============================================================================

static void BM_JamVec3Add(benchmark::State& state)
{
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a + b);
    }
}
BENCHMARK(BM_JamVec3Add)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec3Add(benchmark::State& state)
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a + b);
    }
}
BENCHMARK(BM_DirectXVec3Add)->Unit(benchmark::kNanosecond);

static void BM_JamVec3Sub(benchmark::State& state)
{
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a - b);
    }
}
BENCHMARK(BM_JamVec3Sub)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec3Sub(benchmark::State& state)
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a - b);
    }
}
BENCHMARK(BM_DirectXVec3Sub)->Unit(benchmark::kNanosecond);

static void BM_JamVec3Mul(benchmark::State& state)
{
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a * b);
    }
}
BENCHMARK(BM_JamVec3Mul)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec3Mul(benchmark::State& state)
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a * b);
    }
}
BENCHMARK(BM_DirectXVec3Mul)->Unit(benchmark::kNanosecond);

static void BM_JamVec3Dot(benchmark::State& state)
{
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Dot(b));
    }
}
BENCHMARK(BM_JamVec3Dot)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec3Dot(benchmark::State& state)
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Dot(b));
    }
}
BENCHMARK(BM_DirectXVec3Dot)->Unit(benchmark::kNanosecond);

static void BM_JamVec3Cross(benchmark::State& state)
{
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Cross(b));
    }
}
BENCHMARK(BM_JamVec3Cross)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec3Cross(benchmark::State& state)
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Cross(b));
    }
}
BENCHMARK(BM_DirectXVec3Cross)->Unit(benchmark::kNanosecond);

static void BM_JamVec3Normalize(benchmark::State& state)
{
    Vec3 a(3.0f, 4.0f, 5.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Normalized());
    }
}
BENCHMARK(BM_JamVec3Normalize)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec3Normalize(benchmark::State& state)
{
    Vector3 a(3.0f, 4.0f, 5.0f);
    for (auto _: state)
    {
        a.Normalize();
        benchmark::DoNotOptimize(a);
    }
}
BENCHMARK(BM_DirectXVec3Normalize)->Unit(benchmark::kNanosecond);

static void BM_JamVec3Distance(benchmark::State& state)
{
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Distance(b));
    }
}
BENCHMARK(BM_JamVec3Distance)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec3Distance(benchmark::State& state)
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(Vector3::Distance(a, b));
    }
}
BENCHMARK(BM_DirectXVec3Distance)->Unit(benchmark::kNanosecond);

// ============================================================================
// Vector4 Benchmarks
// ============================================================================

static void BM_JamVec4Add(benchmark::State& state)
{
    Vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a + b);
    }
}
BENCHMARK(BM_JamVec4Add)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec4Add(benchmark::State& state)
{
    DirectX::SimpleMath::Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    DirectX::SimpleMath::Vector4 b(5.0f, 6.0f, 7.0f, 8.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a + b);
    }
}
BENCHMARK(BM_DirectXVec4Add)->Unit(benchmark::kNanosecond);

static void BM_JamVec4Dot(benchmark::State& state)
{
    Vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Dot(b));
    }
}
BENCHMARK(BM_JamVec4Dot)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec4Dot(benchmark::State& state)
{
    DirectX::SimpleMath::Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    DirectX::SimpleMath::Vector4 b(5.0f, 6.0f, 7.0f, 8.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Dot(b));
    }
}
BENCHMARK(BM_DirectXVec4Dot)->Unit(benchmark::kNanosecond);

static void BM_JamVec4Normalize(benchmark::State& state)
{
    Vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    for (auto _: state)
    {
        a.Normalize();
        benchmark::DoNotOptimize(a);
    }
}
BENCHMARK(BM_JamVec4Normalize)->Unit(benchmark::kNanosecond);

static void BM_DirectXVec4Normalize(benchmark::State& state)
{
    DirectX::SimpleMath::Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    for (auto _: state)
    {
        a.Normalize();
        benchmark::DoNotOptimize(a);
    }
}
BENCHMARK(BM_DirectXVec4Normalize)->Unit(benchmark::kNanosecond);

// ============================================================================
// Matrix4x4 Benchmarks
// ============================================================================

static void BM_JamMtx4Add(benchmark::State& state)
{
    Mtx4 a = Mtx4::kIdentity;
    Mtx4 b = Mtx4::kIdentity;
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a + b);
    }
}
BENCHMARK(BM_JamMtx4Add)->Unit(benchmark::kNanosecond);

static void BM_DirectXMatrixAdd(benchmark::State& state)
{
    DirectX::SimpleMath::Matrix a = DirectX::SimpleMath::Matrix::Identity;
    DirectX::SimpleMath::Matrix b = DirectX::SimpleMath::Matrix::Identity;
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a + b);
    }
}
BENCHMARK(BM_DirectXMatrixAdd)->Unit(benchmark::kNanosecond);

static void BM_JamMtx4Mul(benchmark::State& state)
{
    Mtx4 a = Mtx4::CreateRotationZ(0.5f);
    Mtx4 b = Mtx4::CreateTranslation(Vec3(1.0f, 2.0f, 3.0f));
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a * b);
    }
}
BENCHMARK(BM_JamMtx4Mul)->Unit(benchmark::kNanosecond);

static void BM_DirectXMatrixMul(benchmark::State& state)
{
    DirectX::SimpleMath::Matrix a = DirectX::SimpleMath::Matrix::CreateRotationZ(0.5f);
    DirectX::SimpleMath::Matrix b = DirectX::SimpleMath::Matrix::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f));
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a * b);
    }
}
BENCHMARK(BM_DirectXMatrixMul)->Unit(benchmark::kNanosecond);

static void BM_JamMtx4Transpose(benchmark::State& state)
{
    Mtx4 a = Mtx4::CreateRotationZ(0.5f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Transpose());
    }
}
BENCHMARK(BM_JamMtx4Transpose)->Unit(benchmark::kNanosecond);

static void BM_DirectXMatrixTranspose(benchmark::State& state)
{
    DirectX::SimpleMath::Matrix a = DirectX::SimpleMath::Matrix::CreateRotationZ(0.5f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Transpose());
    }
}
BENCHMARK(BM_DirectXMatrixTranspose)->Unit(benchmark::kNanosecond);

static void BM_JamMtx4Inverse(benchmark::State& state)
{
    Mtx4 a = Mtx4::CreateRotationZ(0.5f) * Mtx4::CreateTranslation(Vec3(1.0f, 2.0f, 3.0f));
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Inverse());
    }
}
BENCHMARK(BM_JamMtx4Inverse)->Unit(benchmark::kNanosecond);

static void BM_DirectXMatrixInverse(benchmark::State& state)
{
    DirectX::SimpleMath::Matrix a = DirectX::SimpleMath::Matrix::CreateRotationZ(0.5f) * DirectX::SimpleMath::Matrix::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f));
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Invert());
    }
}
BENCHMARK(BM_DirectXMatrixInverse)->Unit(benchmark::kNanosecond);

static void BM_JamMtx4VecTransform(benchmark::State& state)
{
    Mtx4 m = Mtx4::CreateRotationZ(0.5f) * Mtx4::CreateTranslation(Vec3(1.0f, 2.0f, 3.0f));
    Vec3 v(1.0f, 2.0f, 3.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(m.Mult(v));
    }
}
BENCHMARK(BM_JamMtx4VecTransform)->Unit(benchmark::kNanosecond);

static void BM_DirectXMatrixVecTransform(benchmark::State& state)
{
    DirectX::SimpleMath::Matrix m = DirectX::SimpleMath::Matrix::CreateRotationZ(0.5f) * DirectX::SimpleMath::Matrix::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f));
    Vector3                     v(1.0f, 2.0f, 3.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(DirectX::SimpleMath::Vector3::Transform(v, m));
    }
}
BENCHMARK(BM_DirectXMatrixVecTransform)->Unit(benchmark::kNanosecond);

// ============================================================================
// Quaternion Benchmarks
// ============================================================================

static void BM_JamQuatMul(benchmark::State& state)
{
    Quat a = Quat::FromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), 0.5f);
    Quat b = Quat::FromAxisAngle(Vec3(1.0f, 0.0f, 0.0f), 0.3f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a * b);
    }
}
BENCHMARK(BM_JamQuatMul)->Unit(benchmark::kNanosecond);

static void BM_DirectXQuatMul(benchmark::State& state)
{
    DirectX::SimpleMath::Quaternion a = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), 0.5f);
    DirectX::SimpleMath::Quaternion b = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), 0.3f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a * b);
    }
}
BENCHMARK(BM_DirectXQuatMul)->Unit(benchmark::kNanosecond);

static void BM_JamQuatNormalize(benchmark::State& state)
{
    Quat a(1.0f, 2.0f, 3.0f, 4.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Normalized());
    }
}
BENCHMARK(BM_JamQuatNormalize)->Unit(benchmark::kNanosecond);

static void BM_DirectXQuatNormalize(benchmark::State& state)
{
    DirectX::SimpleMath::Quaternion a(1.0f, 2.0f, 3.0f, 4.0f);
    for (auto _: state)
    {
        a.Normalize();
        benchmark::DoNotOptimize(a);
    }
}
BENCHMARK(BM_DirectXQuatNormalize)->Unit(benchmark::kNanosecond);

static void BM_JamQuatConjugate(benchmark::State& state)
{
    Quat a = Quat::FromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), 0.5f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Conjugate());
    }
}
BENCHMARK(BM_JamQuatConjugate)->Unit(benchmark::kNanosecond);

static void BM_DirectXQuatConjugate(benchmark::State& state)
{
    DirectX::SimpleMath::Quaternion a = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), 0.5f);
    for (auto _: state)
    {
        a.Conjugate();
        benchmark::DoNotOptimize(a);
    }
}
BENCHMARK(BM_DirectXQuatConjugate)->Unit(benchmark::kNanosecond);

static void BM_JamQuatRotateVec3(benchmark::State& state)
{
    Quat q = Quat::FromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), 0.5f);
    Vec3 v(1.0f, 2.0f, 3.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(q.Rotate(v));
    }
}
BENCHMARK(BM_JamQuatRotateVec3)->Unit(benchmark::kNanosecond);

static void BM_DirectXQuatRotateVec3(benchmark::State& state)
{
    DirectX::SimpleMath::Quaternion q = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), 0.5f);
    Vector3                         v(1.0f, 2.0f, 3.0f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(DirectX::SimpleMath::Vector3::Transform(v, q));
    }
}
BENCHMARK(BM_DirectXQuatRotateVec3)->Unit(benchmark::kNanosecond);

static void BM_JamQuatSlerp(benchmark::State& state)
{
    Quat a = Quat::FromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), 0.5f);
    Quat b = Quat::FromAxisAngle(Vec3(1.0f, 0.0f, 0.0f), 0.3f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(a.Slerp(b, 0.5f));
    }
}
BENCHMARK(BM_JamQuatSlerp)->Unit(benchmark::kNanosecond);

static void BM_DirectXQuatSlerp(benchmark::State& state)
{
    DirectX::SimpleMath::Quaternion a = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), 0.5f);
    DirectX::SimpleMath::Quaternion b = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), 0.3f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(DirectX::SimpleMath::Quaternion::Slerp(a, b, 0.5f));
    }
}
BENCHMARK(BM_DirectXQuatSlerp)->Unit(benchmark::kNanosecond);
