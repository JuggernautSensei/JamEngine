#include "pch.h"
#include <limits>
#include <cmath>

using namespace Jam;

// --- 1. Math Constants & Type Limits ---
TEST(MathTest, ConstantsAndLimits)
{
    // Basic Constants
    static_assert(k_pi > 3.14f && k_pi < 3.15f);
    static_assert(ToDeg(k_pi) == 180.0f);
    static_assert(ToRad(180.0f) == k_pi);

    // Float Bit Manipulation
    static_assert(k_floatSignMask == 0x80000000);
    EXPECT_TRUE(IsInfinite(k_inf));

    // Limits Template
    static_assert(Limits<Int32>::k_max == 2147483647);
    static_assert(Limits<float>::k_lowest == -FLT_MAX);
    static_assert(Limits<UInt8>::k_max == 255);
}

// --- 2. Bit & Power Operations ---
TEST(MathTest, BitPowerOps)
{
    // Power of 2
    static_assert(IsPowerOf2(1024));
    static_assert(!IsPowerOf2(7));
    static_assert(NextPowerOf2(5) == 8);
    static_assert(NextPowerOf2(16) == 16);

    // Log2 for Integers
    static_assert(Log2(UInt32(8)) == 3);
    static_assert(Log2(UInt32(1024)) == 10);
    static_assert(Log2(UInt64(1ULL << 40)) == 40);
}

// --- 3. Numeric Functions (Scalar) ---
TEST(MathTest, ScalarFunctions)
{
    // Basic Math
    static_assert(Abs(-5.5f) == 5.5f);
    static_assert(Max(10, 20, 5, 30) == 30);
    static_assert(Min(10, 20, 5, 30) == 5);
    static_assert(Clamp(15, 0, 10) == 10);

    // Float rounding
    static_assert(Floor(1.9f) == 1.0f);
    static_assert(Floor(-1.1f) == -2.0f);
    static_assert(Ceil(1.1f) == 2.0f);
    static_assert(Round(1.6f) == 2.0f);
    static_assert(Fract(1.25f) == 0.25f);

    // Comparison
    static_assert(IsZeroApprox(1e-7f));
    static_assert(IsEqualApprox(1.000001f, 1.0f));
}

// --- 4. Advanced Math (Log, Exp, Trig) ---
TEST(MathTest, AdvancedFunctions)
{
    // Constexpr Log/Exp (수정됨: 정밀도 오차 고려하여 constexpr 내에서는 근사치 확인)
    static_assert(IsEqualApprox(Log(k_e), 1.0f, 1e-5f));
    static_assert(IsEqualApprox(Exp(1.0f), k_e, 1e-5f));
    static_assert(IsEqualApprox(Power(2.0f, 3.0f), 8.0f, 1e-4f));

    // Trig
    static_assert(IsEqualApprox(Sin(k_halfPi), 1.0f));
    static_assert(IsEqualApprox(Cos(k_pi), -1.0f));
    static_assert(IsEqualApprox(Tan(0.0f), 0.0f));

    // Inverse Trig
    EXPECT_NEAR(Atan2(1.0f, 1.0f), k_pi / 4.0f, k_epsilon);
    EXPECT_NEAR(Acos(1.0f), 0.0f, k_epsilon);
}

// --- 5. Vector Basics & Constexpr ---
TEST(MathTest, VectorBasics)
{
    // Vec2 Constexpr
    constexpr Vec2 v2 = { 3.0f, 4.0f };
    static_assert(v2.x == 3.0f);
    static_assert(LengthSq(v2) == 25.0f);
    static_assert(Dot(v2, vec2::k_unitX) == 3.0f);

    // Vec3 Constexpr
    constexpr Vec3 v3a   = { 1.0f, 0.0f, 0.0f };
    constexpr Vec3 v3b   = { 0.0f, 1.0f, 0.0f };
    constexpr Vec3 cross = Cross(v3a, v3b);
    static_assert(cross.z == 1.0f);

    // Vec4
    constexpr Vec4 v4 = vec4::k_one;
    static_assert(LengthSq(v4) == 4.0f);
}

// --- 6. SIMD vs Constexpr Branching ---
TEST(MathTest, SIMDVerification)
{
    // 이 테스트는 런타임에 실행되므로 SIM드 경로(DirectXMath)를 태움
    Vec3 a = { 1.0f, 2.0f, 3.0f };
    Vec3 b = { 4.0f, 5.0f, 6.0f };

    float dotResult = Dot(a, b);
    EXPECT_NEAR(dotResult, 32.0f, k_epsilon);

    Vec3 norm = Normalized(a);
    EXPECT_NEAR(Length(norm), 1.0f, k_epsilon);

    // InvSqrt SSE check
    float invLen = InvLength(a);
    EXPECT_NEAR(invLen, 1.0f / std::sqrt(14.0f), 1e-3f);
}

// --- 7. NumericCast & Overflow ---
TEST(MathTest, CastLogic)
{
    // Safe cast
    static_assert(NumericCast<Int32>(100.0f) == 100);

    // Overflow check (Debug build에서는 JAM_ASSERT 발생)
    EXPECT_TRUE(CheckOverflow<Int8>(200));
    EXPECT_FALSE(CheckOverflow<Int32>(1000));

    // Enum cast
    enum class TestEnum : UInt32
    {
        Value = 10
    };
    static_assert(NumericCast<Int32>(TestEnum::Value) == 10);
}

// --- 8. Random Numbers ---
TEST(MathTest, RandomGeneration)
{
    // 범위 검증
    int randI = MakeRandom<int>(1, 10);
    EXPECT_GE(randI, 1);
    EXPECT_LE(randI, 10);

    float randF = MakeRandom<float>(0.0f, 1.0f);
    EXPECT_GE(randF, 0.0f);
    EXPECT_LE(randF, 1.0f);

    // 시드 재현성 검증 (수정됨)
    // r1을 생성하며 시드를 42로 고정
    int r1 = MakeRandom<int>(0, 1000, 42);

    // r2를 생성하기 전에 다른 시드를 한 번 넣어 g_randomSeed를 강제로 변경
    (void)MakeRandom<int>(0, 1000, 999);

    // 다시 42를 넣으면 내부에서 g_randomGenerator.seed(42)가 호출됨
    int r2 = MakeRandom<int>(0, 1000, 42);

    EXPECT_EQ(r1, r2);
}

// --- 9. Coordinate System & Conversion ---
TEST(MathTest, CoordinateConversion)
{
    // Degrees/Radians
    constexpr Vec3 degs = { 180.0f, 90.0f, 0.0f };
    constexpr Vec3 rads = ToRad(degs);
    static_assert(IsEqualApprox(rads.x, k_pi));
    static_assert(IsEqualApprox(rads.y, k_halfPi));

    // Vec Conversion
    constexpr Vec4 v4 = { 1, 2, 3, 4 };
    constexpr Vec3 v3 = Vec3From(v4);
    static_assert(v3.x == 1 && v3.z == 3);
}

// --- 10. Matrix & Identity ---
TEST(MathTest, MatrixBasics)
{
    static_assert(mtx4::k_identity._11 == 1.0f);
    static_assert(mtx4::k_identity._12 == 0.0f);

    // Identity quat
    static_assert(quat::k_identity.w == 1.0f);
    static_assert(quat::k_identity.x == 0.0f);
}

// --- 11. Vec2i Operations ---
TEST(MathTest, Vec2iOps)
{
    constexpr Vec2i a(5, 10);
    constexpr Vec2i b(2, 3);
    constexpr Vec2i sum = a + b;
    static_assert(sum.x == 7 && sum.y == 13);

    constexpr Vec2i diff = a - b;
    static_assert(diff.x == 3 && diff.y == 7);

    constexpr Vec2i scaled = b * 2;
    static_assert(scaled.x == 4 && scaled.y == 6);

    Vec2i c = a;
    c += b;
    EXPECT_EQ(c.x, 7);
    EXPECT_EQ(c.y, 13);

    c -= b;
    EXPECT_EQ(c.x, 5);
    EXPECT_EQ(c.y, 10);

    c *= 2;
    EXPECT_EQ(c.x, 10);
    EXPECT_EQ(c.y, 20);

    EXPECT_EQ(Vec2i::k_zero.x, 0);
    EXPECT_EQ(Vec2i::k_one.x, 1);
}

// --- 12. Additional Scalar Functions (Sign, Mod, Wrap, Div, Lerp) ---
TEST(MathTest, AdditionalScalars)
{
    // Sign / SignBit / CopySign
    static_assert(Sign(10.0f) == 1.0f);
    static_assert(Sign(-5.0f) == -1.0f);
    static_assert(Sign(0.0f) == 0.0f);
    static_assert(SignBit(-5.0f));
    static_assert(!SignBit(5.0f));
    static_assert(CopySign(1.0f, -5.0f) == -1.0f);

    // Trunc / Mod
    static_assert(Trunc(1.5f) == 1.0f);
    static_assert(Trunc(-1.5f) == -1.0f);
    static_assert(Abs(Mod(5.5f, 2.0f) - 1.5f) < k_epsilon);

    // Lerp
    static_assert(Lerp(0.0f, 10.0f, 0.5f) == 5.0f);
    static_assert(Lerp(0.0f, 10.0f, 0.1f) == 1.0f);
    static_assert(Lerp(10.0f, 0.0f, 0.25f) == 7.5f);

    // Square
    static_assert(Square(3.0f) == 9.0f);
    static_assert(Square(-4.0f) == 16.0f);

    // Wrap
    // Wrap(x, min, max): range = max-min.
    EXPECT_NEAR(Wrap(12.0f, 0.0f, 10.0f), 2.0f, k_epsilon);
    EXPECT_NEAR(Wrap(-2.0f, 0.0f, 10.0f), 8.0f, k_epsilon);
    EXPECT_NEAR(Wrap(5.0f, 0.0f, 10.0f), 5.0f, k_epsilon);
    EXPECT_NEAR(Wrap(10.0f, 0.0f, 10.0f), 0.0f, k_epsilon);

    // Div
    static_assert(Div(10, 2) == 5);
    static_assert(Div(10.0f, 2.0f) == 5.0f);
}

// --- 13. Vector Distance, Abs, Normalize ---
TEST(MathTest, VectorExtras)
{
    // Distance
    constexpr Vec2 p1 = { 0.f, 0.f }, p2 = { 3.f, 4.f };

    static_assert(IsEqualApprox(DistanceSq(p1, p2), 25.0f));
    static_assert(IsEqualApprox(Distance(p1, p2), 5.0f));

    Vec3 p3 = { 0.f, 0.f, 0.f }, p4 = { 1.f, 2.f, 2.f };   // len = 3
    EXPECT_FLOAT_EQ(Distance(p3, p4), 3.0f);
    EXPECT_FLOAT_EQ(DistanceSq(p3, p4), 9.0f);

    // Abs
    constexpr Vec2 vn    = { -1.f, -2.f };
    constexpr Vec2 vnAbs = Abs(vn);
    static_assert(vnAbs.x == 1.f && vnAbs.y == 2.f);

    // Normalized
    Vec3 v = { 10.f, 0.f, 0.f };
    Vec3 n = Normalized(v);
    EXPECT_NEAR(n.x, 1.0f, k_epsilon);
    EXPECT_NEAR(n.y, 0.0f, k_epsilon);

    // Normalize zero vector (check it handles safe or returns zero)
    // Math.h: if (IsZeroApprox(lenSq)) return k_zero;
    Vec3 zero = { 0.f, 0.f, 0.f };
    Vec3 nz   = Normalized(zero);
    EXPECT_FLOAT_EQ(nz.x, 0.f);
}

// --- 14. Float16 Conversion ---
TEST(MathTest, HalfFloat)
{
    float   val  = 1.0f;
    Float16 h    = ToFloat16(val);   // assumes implemented via DirectX
    float   val2 = ToFloat32(h);
    EXPECT_FLOAT_EQ(val, val2);

    float large = 65504.0f;   // Max half float approx
    EXPECT_NEAR(ToFloat32(ToFloat16(large)), large, 10.0f);
}

// --- 15. Geometric Utilities & SinCos ---
TEST(MathTest, GeometricUtils)
{
    // SinCosApprox
    SinCos sc = SinCosApprox(0.0f);
    EXPECT_NEAR(sc.sin, 0.0f, k_epsilon);
    EXPECT_NEAR(sc.cos, 1.0f, k_epsilon);

    sc = SinCosApprox(k_halfPi);
    EXPECT_NEAR(sc.sin, 1.0f, 1e-3f);
    EXPECT_NEAR(sc.cos, 0.0f, 1e-3f);

    // Euler / Forward
    Vec3 forward = vec3::k_forward;   // 0,0,1, typically yaw=0, pitch=0
    Vec3 euler   = EulerFrom(forward);
    // Implementation: cx=0, cy=1 -> return {0,0,0}
    EXPECT_NEAR(euler.x, 0.f, k_epsilon);
    EXPECT_NEAR(euler.y, 0.f, k_epsilon);
    EXPECT_NEAR(euler.z, 0.f, k_epsilon);

    Vec3 right = vec3::k_right;   // 1,0,0
    euler      = EulerFrom(right);
    // Expected yaw = pi/2
    EXPECT_NEAR(euler.y, k_halfPi, k_epsilon);

    Vec3 fwd = ForwardFrom({ 0.f, k_halfPi, 0.f });
    // Should be close to 1,0,0
    EXPECT_NEAR(fwd.x, 1.f, 1e-3f);
    EXPECT_NEAR(fwd.z, 0.f, 1e-3f);
}

// --- 16. Edge Cases & Special Values ---
TEST(MathTest, ComplexEdgeCases)
{
    // Log Edge Cases
    EXPECT_TRUE(IsInfinite(Log(0.0f)));    // Should be -inf
    EXPECT_TRUE(std::isnan(Log(-1.0f)));   // Should be NaN

    // Exp Edge Cases
    EXPECT_EQ(Exp(-1000.0f), 0.0f);

    // Sqrt Negatives
    EXPECT_TRUE(std::isnan(Sqrt(-1.0f)));

    // Atan2(0,0) -> 0
    EXPECT_EQ(Atan2(0.f, 0.f), 0.0f);

    // Division by zero check (IsInfinite)
    // Div() asserts, so we can't test unless we use EXPECT_DEBUG_DEATH (if gtest supported and enabled)
    // Here we just test large numbers safety
    EXPECT_FALSE(CheckOverflow<Int8>(127));
    EXPECT_TRUE(CheckOverflow<Int8>(128));
}

// --- 17. Matrix Generators (Smoke Test) ---
TEST(MathTest, MatrixSmokeTest)
{
    // Just ensure they run and produce valid numbers (not NaNs)
    Mtx4 ortho = CreateOrthoMatrix(800.f, 600.f, 0.1f, 1000.f);
    EXPECT_FALSE(std::isnan(ortho._11));
    EXPECT_FALSE(std::isnan(ortho._44));

    Mtx4 persp = CreatePerspectiveMatrix(ToRad(45.f), 1.77f, 0.1f, 1000.f);
    EXPECT_FALSE(std::isnan(persp._11));
    // _11 = 1 / (aspect * tan(fov/2))
    EXPECT_GT(persp._11, 0.0f);
    EXPECT_EQ(persp._44, 0.0f);   // Perspective projection usually has w-transform
}