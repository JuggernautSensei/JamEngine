// =============================================================================
// MathTest.cpp
// Jam 수학 라이브러리 단위 테스트
//
// 좌표계 규약: 왼손 좌표계 (Left-Handed) / 행 기준 벡터 (Row-Major)
//   - 변환: v' = v * M  (벡터가 행렬의 왼쪽)
//   - 합성: M_total = M1 * M2  (M1 먼저, 그 다음 M2 적용)
//   - 기저: +X = Right, +Y = Up, +Z = Forward
//   - 회전 부호: 양의 각도는 해당 축을 기준으로 right-hand rule 반대
//       RotX: +y -> +z
//       RotY: +z -> +x
//       RotZ: +x -> +y
//   - 투영: near -> NDC z = 0, far -> NDC z = 1 (DirectX 스타일)
//
// 의존: Google Test (gtest)
// =============================================================================

#include "pch.h"
#include <gtest/gtest.h>

#include "Math.h"

using namespace Jam;

namespace
{
// 기본 허용 오차 (SIMD rsqrt 정밀도 고려)
constexpr float kEps      = 1e-4f;
constexpr float kLooseEps = 1e-3f;

// 스칼라 근사 비교
bool Approx(const float _a, const float _b, const float _eps = kEps)
{
    return Jam::Abs(_a - _b) <= _eps;
}

// 각도 상수
constexpr float kHalfPiF    = Jam::kHalfPi;
constexpr float kPiF        = Jam::kPi;
constexpr float kQuarterPiF = Jam::kPi * 0.25f;
}   // namespace

// =============================================================================
//  Vec2i 테스트
// =============================================================================

TEST(Vec2i, DefaultConstructor)
{
    constexpr Vec2i v;
    EXPECT_EQ(v.x, 0);
    EXPECT_EQ(v.y, 0);
}

TEST(Vec2i, ValueConstructor)
{
    constexpr Vec2i v { 3, -7 };
    EXPECT_EQ(v.x, 3);
    EXPECT_EQ(v.y, -7);
}

TEST(Vec2i, UniformConstructor)
{
    constexpr Vec2i v { 5 };
    EXPECT_EQ(v.x, 5);
    EXPECT_EQ(v.y, 5);
}

TEST(Vec2i, UnionAliases)
{
    Vec2i v { 640, 480 };
    EXPECT_EQ(v.u, 640);
    EXPECT_EQ(v.v, 480);
    EXPECT_EQ(v.width, 640);
    EXPECT_EQ(v.height, 480);
    EXPECT_EQ(v.e[0], 640);
    EXPECT_EQ(v.e[1], 480);
}

TEST(Vec2i, Add)
{
    const Vec2i a { 1, 2 }, b { 3, 4 };
    const Vec2i c = a + b;
    EXPECT_EQ(c.x, 4);
    EXPECT_EQ(c.y, 6);
}

TEST(Vec2i, Subtract)
{
    const Vec2i a { 5, 7 }, b { 2, 3 };
    const Vec2i c = a - b;
    EXPECT_EQ(c.x, 3);
    EXPECT_EQ(c.y, 4);
}

TEST(Vec2i, Multiply)
{
    const Vec2i a { 2, 3 }, b { 4, 5 };
    const Vec2i c = a * b;
    EXPECT_EQ(c.x, 8);
    EXPECT_EQ(c.y, 15);
}

TEST(Vec2i, Divide)
{
    const Vec2i a { 10, 20 }, b { 2, 5 };
    const Vec2i c = a / b;
    EXPECT_EQ(c.x, 5);
    EXPECT_EQ(c.y, 4);
}

TEST(Vec2i, ScalarMultiply)
{
    const Vec2i a { 2, 3 };
    const Vec2i c = a * 4;
    EXPECT_EQ(c.x, 8);
    EXPECT_EQ(c.y, 12);
}

TEST(Vec2i, ScalarDivide)
{
    const Vec2i a { 8, 12 };
    const Vec2i c = a / 4;
    EXPECT_EQ(c.x, 2);
    EXPECT_EQ(c.y, 3);
}

TEST(Vec2i, Negate)
{
    const Vec2i a { 3, -5 };
    const Vec2i c = -a;
    EXPECT_EQ(c.x, -3);
    EXPECT_EQ(c.y, 5);
}

TEST(Vec2i, CompoundAssignments)
{
    Vec2i a { 1, 2 };
    a += Vec2i { 3, 4 };
    EXPECT_EQ(a, (Vec2i { 4, 6 }));

    a -= Vec2i { 1, 1 };
    EXPECT_EQ(a, (Vec2i { 3, 5 }));

    a *= Vec2i { 2, 2 };
    EXPECT_EQ(a, (Vec2i { 6, 10 }));

    a /= Vec2i { 2, 5 };
    EXPECT_EQ(a, (Vec2i { 3, 2 }));

    a *= 3;
    EXPECT_EQ(a, (Vec2i { 9, 6 }));

    a /= 3;
    EXPECT_EQ(a, (Vec2i { 3, 2 }));
}

TEST(Vec2i, Abs)
{
    const Vec2i a { -3, 5 };
    const Vec2i c = a.Abs();
    EXPECT_EQ(c.x, 3);
    EXPECT_EQ(c.y, 5);
}

TEST(Vec2i, Min)
{
    const Vec2i a { 2, 8 }, b { 5, 3 };
    const Vec2i c = a.Min(b);
    EXPECT_EQ(c.x, 2);
    EXPECT_EQ(c.y, 3);
}

TEST(Vec2i, Max)
{
    const Vec2i a { 2, 8 }, b { 5, 3 };
    const Vec2i c = a.Max(b);
    EXPECT_EQ(c.x, 5);
    EXPECT_EQ(c.y, 8);
}

TEST(Vec2i, Clamp)
{
    const Vec2i v { 10, -5 };
    const Vec2i c = v.Clamp({ 0, 0 }, { 8, 8 });
    EXPECT_EQ(c.x, 8);
    EXPECT_EQ(c.y, 0);
}

TEST(Vec2i, Equality)
{
    const Vec2i a { 1, 2 }, b { 1, 2 }, c { 3, 4 };
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(Vec2i, StaticConstants)
{
    EXPECT_EQ(Vec2i::kZero, (Vec2i { 0, 0 }));
    EXPECT_EQ(Vec2i::kOne, (Vec2i { 1, 1 }));
    EXPECT_EQ(Vec2i::kUnitX, (Vec2i { 1, 0 }));
    EXPECT_EQ(Vec2i::kUnitY, (Vec2i { 0, 1 }));
}

// =============================================================================
//  Vec2 테스트
// =============================================================================

TEST(Vec2, DefaultConstructor)
{
    constexpr Vec2 v;
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
}

TEST(Vec2, ValueConstructor)
{
    constexpr Vec2 v { 1.5f, -2.5f };
    EXPECT_FLOAT_EQ(v.x, 1.5f);
    EXPECT_FLOAT_EQ(v.y, -2.5f);
}

TEST(Vec2, UniformConstructor)
{
    constexpr Vec2 v { 3.14f };
    EXPECT_FLOAT_EQ(v.x, 3.14f);
    EXPECT_FLOAT_EQ(v.y, 3.14f);
}

TEST(Vec2, Length)
{
    const Vec2 v { 3.f, 4.f };
    EXPECT_TRUE(Approx(v.Length(), 5.f));
}

TEST(Vec2, LengthSq)
{
    const Vec2 v { 3.f, 4.f };
    EXPECT_FLOAT_EQ(v.LengthSq(), 25.f);
}

TEST(Vec2, Distance)
{
    const Vec2 a { 0.f, 0.f }, b { 3.f, 4.f };
    EXPECT_TRUE(Approx(a.Distance(b), 5.f));
    EXPECT_FLOAT_EQ(a.DistanceSq(b), 25.f);
}

TEST(Vec2, Normalize)
{
    const Vec2 v { 3.f, 4.f };
    const Vec2 n = v.Normalized();
    EXPECT_TRUE(Approx(n.Length(), 1.f));
    EXPECT_TRUE(Approx(n.x, 0.6f));
    EXPECT_TRUE(Approx(n.y, 0.8f));
}

TEST(Vec2, Dot)
{
    EXPECT_FLOAT_EQ(Vec2(1.f, 0.f).Dot({ 0.f, 1.f }), 0.f);
    EXPECT_FLOAT_EQ(Vec2(2.f, 3.f).Dot({ 4.f, 5.f }), 23.f);
}

TEST(Vec2, Cross)
{
    // 2D 외적 = z 성분 스칼라
    EXPECT_FLOAT_EQ(Vec2(1.f, 0.f).Cross({ 0.f, 1.f }), 1.f);
    EXPECT_FLOAT_EQ(Vec2(0.f, 1.f).Cross({ 1.f, 0.f }), -1.f);
}

TEST(Vec2, Lerp)
{
    const Vec2 a { 0.f, 0.f }, b { 10.f, 20.f };
    const Vec2 c = a.Lerp(b, 0.5f);
    EXPECT_TRUE(Approx(c.x, 5.f));
    EXPECT_TRUE(Approx(c.y, 10.f));
}

TEST(Vec2, Reflect)
{
    // 법선이 (0,1)인 수평면에 (1,-1)이 부딪혀 반사 -> (1,1)
    const Vec2 v { 1.f, -1.f };
    const Vec2 n { 0.f, 1.f };
    const Vec2 r = v.Reflect(n);
    EXPECT_TRUE(Approx(r.x, 1.f));
    EXPECT_TRUE(Approx(r.y, 1.f));
}

TEST(Vec2, Project)
{
    const Vec2 v { 3.f, 4.f }, onto { 1.f, 0.f };
    const Vec2 p = v.Project(onto);
    EXPECT_TRUE(Approx(p.x, 3.f));
    EXPECT_TRUE(Approx(p.y, 0.f));
}

TEST(Vec2, Angle)
{
    EXPECT_TRUE(Approx(Vec2(1.f, 0.f).Angle({ 0.f, 1.f }), kHalfPiF));
    EXPECT_TRUE(Approx(Vec2(1.f, 0.f).Angle({ 1.f, 0.f }), 0.f));
}

TEST(Vec2, IsZeroApprox)
{
    EXPECT_TRUE(Vec2(0.f, 0.f).IsZeroApprox());
    EXPECT_FALSE(Vec2(0.01f, 0.f).IsZeroApprox());
}

TEST(Vec2, IsEqualApprox)
{
    const Vec2 a { 1.f, 2.f };
    const Vec2 b { 1.f + 1e-7f, 2.f - 1e-7f };
    EXPECT_TRUE(a.IsEqualApprox(b));
}

TEST(Vec2, StaticConstants)
{
    EXPECT_TRUE(Vec2::kZero.IsZeroApprox());
    EXPECT_TRUE(Vec2::kOne.IsEqualApprox({ 1.f, 1.f }));
    EXPECT_TRUE(Vec2::kUnitX.IsEqualApprox({ 1.f, 0.f }));
    EXPECT_TRUE(Vec2::kUnitY.IsEqualApprox({ 0.f, 1.f }));
    EXPECT_TRUE(Vec2::kRight.IsEqualApprox(Vec2::kUnitX));
    EXPECT_TRUE(Vec2::kUp.IsEqualApprox(Vec2::kUnitY));
}

// =============================================================================
//  Vec3 테스트
// =============================================================================

TEST(Vec3, DefaultConstructor)
{
    constexpr Vec3 v;
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
    EXPECT_FLOAT_EQ(v.z, 0.f);
}

TEST(Vec3, ValueConstructor)
{
    constexpr Vec3 v { 1.f, 2.f, 3.f };
    EXPECT_FLOAT_EQ(v.x, 1.f);
    EXPECT_FLOAT_EQ(v.y, 2.f);
    EXPECT_FLOAT_EQ(v.z, 3.f);
}

TEST(Vec3, Vec2AndZ)
{
    constexpr Vec3 v {
        Vec2 { 1.f, 2.f },
        3.f
    };
    EXPECT_FLOAT_EQ(v.x, 1.f);
    EXPECT_FLOAT_EQ(v.y, 2.f);
    EXPECT_FLOAT_EQ(v.z, 3.f);
}

TEST(Vec3, UnionAliases)
{
    Vec3 v { 1.f, 2.f, 3.f };
    EXPECT_FLOAT_EQ(v.r, 1.f);
    EXPECT_FLOAT_EQ(v.g, 2.f);
    EXPECT_FLOAT_EQ(v.b, 3.f);
    EXPECT_FLOAT_EQ(v.pitch, 1.f);
    EXPECT_FLOAT_EQ(v.yaw, 2.f);
    EXPECT_FLOAT_EQ(v.roll, 3.f);
    EXPECT_FLOAT_EQ(v.e[0], 1.f);
    EXPECT_FLOAT_EQ(v.e[1], 2.f);
    EXPECT_FLOAT_EQ(v.e[2], 3.f);
}

TEST(Vec3, Length)
{
    const Vec3 v { 1.f, 2.f, 2.f };   // sqrt(1+4+4)=3
    EXPECT_TRUE(Approx(v.Length(), 3.f));
    EXPECT_FLOAT_EQ(v.LengthSq(), 9.f);
}

TEST(Vec3, Distance)
{
    const Vec3 a { 1.f, 2.f, 3.f }, b { 4.f, 6.f, 3.f };
    EXPECT_TRUE(Approx(a.Distance(b), 5.f));
}

TEST(Vec3, Normalize)
{
    Vec3 v { 0.f, 0.f, 5.f };
    v.Normalize();
    EXPECT_TRUE(Approx(v.Length(), 1.f));
    EXPECT_TRUE(Approx(v.z, 1.f));

    const Vec3 v2 { 3.f, 4.f, 0.f };
    const Vec3 n = v2.Normalized();
    EXPECT_TRUE(Approx(n.Length(), 1.f));
    EXPECT_TRUE(Approx(n.x, 0.6f));
    EXPECT_TRUE(Approx(n.y, 0.8f));
}

TEST(Vec3, Dot)
{
    EXPECT_FLOAT_EQ(Vec3(1.f, 0.f, 0.f).Dot({ 0.f, 1.f, 0.f }), 0.f);
    EXPECT_FLOAT_EQ(Vec3(1.f, 2.f, 3.f).Dot({ 4.f, 5.f, 6.f }), 32.f);
}

TEST(Vec3, Cross_LeftHandBasis)
{
    // 왼손 좌표계의 표준 기저에서도 수학적 외적은 동일
    //   X × Y = Z,  Y × Z = X,  Z × X = Y
    EXPECT_TRUE(Vec3::kUnitX.Cross(Vec3::kUnitY).IsEqualApprox(Vec3::kUnitZ));
    EXPECT_TRUE(Vec3::kUnitY.Cross(Vec3::kUnitZ).IsEqualApprox(Vec3::kUnitX));
    EXPECT_TRUE(Vec3::kUnitZ.Cross(Vec3::kUnitX).IsEqualApprox(Vec3::kUnitY));

    // 반교환 법칙
    EXPECT_TRUE(Vec3::kUnitY.Cross(Vec3::kUnitX).IsEqualApprox(-Vec3::kUnitZ));
}

TEST(Vec3, Cross_Specific)
{
    const Vec3 a { 2.f, 3.f, 4.f }, b { 5.f, 6.f, 7.f };
    const Vec3 c = a.Cross(b);
    // (3*7-4*6, 4*5-2*7, 2*6-3*5) = (-3, 6, -3)
    EXPECT_TRUE(Approx(c.x, -3.f));
    EXPECT_TRUE(Approx(c.y, 6.f));
    EXPECT_TRUE(Approx(c.z, -3.f));
}

TEST(Vec3, Reflect)
{
    const Vec3 v { 1.f, -1.f, 0.f }, n { 0.f, 1.f, 0.f };
    const Vec3 r = v.Reflect(n);
    EXPECT_TRUE(Approx(r.x, 1.f));
    EXPECT_TRUE(Approx(r.y, 1.f));
    EXPECT_TRUE(Approx(r.z, 0.f));
}

TEST(Vec3, Project)
{
    const Vec3 v { 3.f, 4.f, 5.f }, onto { 1.f, 0.f, 0.f };
    const Vec3 p = v.Project(onto);
    EXPECT_TRUE(Approx(p.x, 3.f));
    EXPECT_TRUE(Approx(p.y, 0.f));
    EXPECT_TRUE(Approx(p.z, 0.f));
}

TEST(Vec3, Lerp)
{
    const Vec3 a { 0.f, 0.f, 0.f };
    const Vec3 b { 10.f, 20.f, 30.f };
    const Vec3 m = a.Lerp(b, 0.5f);
    EXPECT_TRUE(Approx(m.x, 5.f));
    EXPECT_TRUE(Approx(m.y, 10.f));
    EXPECT_TRUE(Approx(m.z, 15.f));
}

TEST(Vec3, Angle)
{
    EXPECT_TRUE(Approx(Vec3::kUnitX.Angle(Vec3::kUnitY), kHalfPiF));
    EXPECT_TRUE(Approx(Vec3::kUnitX.Angle(Vec3::kUnitX), 0.f));
    EXPECT_TRUE(Approx(Vec3::kUnitX.Angle(-Vec3::kUnitX), kPiF, kLooseEps));
}

TEST(Vec3, MinMaxClamp)
{
    const Vec3 a { 1.f, 5.f, 3.f };
    const Vec3 b { 2.f, 4.f, 6.f };
    EXPECT_TRUE(a.Min(b).IsEqualApprox({ 1.f, 4.f, 3.f }));
    EXPECT_TRUE(a.Max(b).IsEqualApprox({ 2.f, 5.f, 6.f }));

    const Vec3 v { 10.f, -10.f, 5.f };
    const Vec3 c = v.Clamp({ 0.f, 0.f, 0.f }, { 8.f, 8.f, 8.f });
    EXPECT_TRUE(c.IsEqualApprox({ 8.f, 0.f, 5.f }));
}

TEST(Vec3, Arithmetic)
{
    const Vec3 a { 1.f, 2.f, 3.f };
    const Vec3 b { 4.f, 5.f, 6.f };
    EXPECT_TRUE((a + b).IsEqualApprox({ 5.f, 7.f, 9.f }));
    EXPECT_TRUE((a - b).IsEqualApprox({ -3.f, -3.f, -3.f }));
    EXPECT_TRUE((a * b).IsEqualApprox({ 4.f, 10.f, 18.f }));
    EXPECT_TRUE((a * 2.f).IsEqualApprox({ 2.f, 4.f, 6.f }));
    EXPECT_TRUE((b / 2.f).IsEqualApprox({ 2.f, 2.5f, 3.f }));
    EXPECT_TRUE((-a).IsEqualApprox({ -1.f, -2.f, -3.f }));
}

TEST(Vec3, CompoundAssignments)
{
    Vec3 a { 1.f, 2.f, 3.f };
    a += Vec3 { 1.f, 1.f, 1.f };
    EXPECT_TRUE(a.IsEqualApprox({ 2.f, 3.f, 4.f }));

    a -= Vec3 { 1.f, 1.f, 1.f };
    EXPECT_TRUE(a.IsEqualApprox({ 1.f, 2.f, 3.f }));

    a *= 2.f;
    EXPECT_TRUE(a.IsEqualApprox({ 2.f, 4.f, 6.f }));

    a /= 2.f;
    EXPECT_TRUE(a.IsEqualApprox({ 1.f, 2.f, 3.f }));
}

TEST(Vec3, StaticConstants)
{
    EXPECT_TRUE(Vec3::kZero.IsZeroApprox());
    EXPECT_TRUE(Vec3::kOne.IsEqualApprox({ 1.f, 1.f, 1.f }));
    EXPECT_TRUE(Vec3::kUnitX.IsEqualApprox({ 1.f, 0.f, 0.f }));
    EXPECT_TRUE(Vec3::kUnitY.IsEqualApprox({ 0.f, 1.f, 0.f }));
    EXPECT_TRUE(Vec3::kUnitZ.IsEqualApprox({ 0.f, 0.f, 1.f }));
    EXPECT_TRUE(Vec3::kRight.IsEqualApprox(Vec3::kUnitX));
    EXPECT_TRUE(Vec3::kUp.IsEqualApprox(Vec3::kUnitY));
    EXPECT_TRUE(Vec3::kForward.IsEqualApprox(Vec3::kUnitZ));   // 왼손: +Z = Forward
}

// =============================================================================
//  Vec4 테스트
// =============================================================================

TEST(Vec4, DefaultConstructor)
{
    constexpr Vec4 v;
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
    EXPECT_FLOAT_EQ(v.z, 0.f);
    EXPECT_FLOAT_EQ(v.w, 0.f);
}

TEST(Vec4, UnionAliases)
{
    Vec4 v { 1.f, 2.f, 3.f, 4.f };
    EXPECT_FLOAT_EQ(v.r, 1.f);
    EXPECT_FLOAT_EQ(v.g, 2.f);
    EXPECT_FLOAT_EQ(v.b, 3.f);
    EXPECT_FLOAT_EQ(v.a, 4.f);

    EXPECT_FLOAT_EQ(v.xyz.x, 1.f);
    EXPECT_FLOAT_EQ(v.xyz.y, 2.f);
    EXPECT_FLOAT_EQ(v.xyz.z, 3.f);
    EXPECT_FLOAT_EQ(v.xy.x, 1.f);
    EXPECT_FLOAT_EQ(v.xy.y, 2.f);
    EXPECT_FLOAT_EQ(v.zw.x, 3.f);
    EXPECT_FLOAT_EQ(v.zw.y, 4.f);
}

TEST(Vec4, Length)
{
    const Vec4 v { 1.f, 2.f, 2.f, 0.f };
    EXPECT_TRUE(Approx(v.Length(), 3.f));
    EXPECT_FLOAT_EQ(v.LengthSq(), 9.f);
}

TEST(Vec4, Normalize)
{
    const Vec4 v { 0.f, 0.f, 5.f, 0.f };
    const Vec4 n = v.Normalized();
    EXPECT_TRUE(Approx(n.Length(), 1.f));
}

TEST(Vec4, Dot)
{
    const Vec4 a { 1.f, 2.f, 3.f, 4.f };
    const Vec4 b { 5.f, 6.f, 7.f, 8.f };
    EXPECT_TRUE(Approx(a.Dot(b), 5.f + 12.f + 21.f + 32.f));
}

TEST(Vec4, Cross_IgnoresW)
{
    // Vec4의 Cross는 w를 0으로 반환하는 3D 외적
    const Vec4 a { 1.f, 0.f, 0.f, 9.f };
    const Vec4 b { 0.f, 1.f, 0.f, 9.f };
    const Vec4 c = a.Cross(b);
    EXPECT_TRUE(Approx(c.x, 0.f));
    EXPECT_TRUE(Approx(c.y, 0.f));
    EXPECT_TRUE(Approx(c.z, 1.f));
    EXPECT_TRUE(Approx(c.w, 0.f));
}

TEST(Vec4, Arithmetic)
{
    const Vec4 a { 1.f, 2.f, 3.f, 4.f };
    const Vec4 b { 2.f, 2.f, 2.f, 2.f };
    EXPECT_TRUE((a + b).IsEqualApprox({ 3.f, 4.f, 5.f, 6.f }));
    EXPECT_TRUE((a - b).IsEqualApprox({ -1.f, 0.f, 1.f, 2.f }));
    EXPECT_TRUE((a * b).IsEqualApprox({ 2.f, 4.f, 6.f, 8.f }));
    EXPECT_TRUE((a * 2.f).IsEqualApprox({ 2.f, 4.f, 6.f, 8.f }));
    EXPECT_TRUE((a / 2.f).IsEqualApprox({ 0.5f, 1.f, 1.5f, 2.f }));
}

TEST(Vec4, MinMaxClamp)
{
    const Vec4 a { 1.f, 5.f, 3.f, 7.f };
    const Vec4 b { 2.f, 4.f, 6.f, 3.f };
    EXPECT_TRUE(a.Min(b).IsEqualApprox({ 1.f, 4.f, 3.f, 3.f }));
    EXPECT_TRUE(a.Max(b).IsEqualApprox({ 2.f, 5.f, 6.f, 7.f }));
}

TEST(Vec4, StaticConstants)
{
    EXPECT_TRUE(Vec4::kZero.IsZeroApprox());
    EXPECT_TRUE(Vec4::kOne.IsEqualApprox({ 1.f, 1.f, 1.f, 1.f }));
    EXPECT_TRUE(Vec4::kUnitW.IsEqualApprox({ 0.f, 0.f, 0.f, 1.f }));
}

// =============================================================================
//  Quat 테스트
// =============================================================================

TEST(Quat, DefaultConstructor)
{
    Quat q;
    EXPECT_FLOAT_EQ(q.x, 0.f);
    EXPECT_FLOAT_EQ(q.y, 0.f);
    EXPECT_FLOAT_EQ(q.z, 0.f);
    EXPECT_FLOAT_EQ(q.w, 0.f);
}

TEST(Quat, Identity)
{
    EXPECT_FLOAT_EQ(Quat::kIdentity.x, 0.f);
    EXPECT_FLOAT_EQ(Quat::kIdentity.y, 0.f);
    EXPECT_FLOAT_EQ(Quat::kIdentity.z, 0.f);
    EXPECT_FLOAT_EQ(Quat::kIdentity.w, 1.f);
    EXPECT_TRUE(Approx(Quat::kIdentity.Length(), 1.f));
}

TEST(Quat, IdentityRotatesNothing)
{
    const Vec3 v { 1.f, 2.f, 3.f };
    EXPECT_TRUE(Quat::kIdentity.Rotate(v).IsEqualApprox(v, kEps));
}

TEST(Quat, FromAxisAngle_Length)
{
    const Quat q = Quat::FromAxisAngle(Vec3::kUnitY, 1.2f);
    EXPECT_TRUE(Approx(q.Length(), 1.f));
}

TEST(Quat, FromAxisAngle_ZeroAngleIsIdentity)
{
    const Quat q = Quat::FromAxisAngle(Vec3::kUnitX, 0.f);
    EXPECT_TRUE(q.IsEqualApprox(Quat::kIdentity, kEps));
}

TEST(Quat, Rotate_XAxis90)
{
    // X축 주위로 +90도: +Y -> +Z (왼손 좌표계 규약)
    const Quat q = Quat::FromAxisAngle(Vec3::kUnitX, kHalfPiF);
    const Vec3 r = q.Rotate(Vec3::kUnitY);
    EXPECT_TRUE(Approx(r.x, 0.f, kEps));
    EXPECT_TRUE(Approx(r.y, 0.f, kEps));
    EXPECT_TRUE(Approx(r.z, 1.f, kEps));
}

TEST(Quat, Rotate_YAxis90)
{
    // Y축 주위로 +90도: +Z -> +X
    const Quat q = Quat::FromAxisAngle(Vec3::kUnitY, kHalfPiF);
    const Vec3 r = q.Rotate(Vec3::kUnitZ);
    EXPECT_TRUE(Approx(r.x, 1.f, kEps));
    EXPECT_TRUE(Approx(r.y, 0.f, kEps));
    EXPECT_TRUE(Approx(r.z, 0.f, kEps));
}

TEST(Quat, Rotate_ZAxis90)
{
    // Z축 주위로 +90도: +X -> +Y
    const Quat q = Quat::FromAxisAngle(Vec3::kUnitZ, kHalfPiF);
    const Vec3 r = q.Rotate(Vec3::kUnitX);
    EXPECT_TRUE(Approx(r.x, 0.f, kEps));
    EXPECT_TRUE(Approx(r.y, 1.f, kEps));
    EXPECT_TRUE(Approx(r.z, 0.f, kEps));
}

TEST(Quat, Rotate_AxisInvariant)
{
    // 회전축 자체는 불변
    const Vec3 axis = Vec3(1.f, 2.f, 3.f).Normalized();
    const Quat q    = Quat::FromAxisAngle(axis, 0.7f);
    const Vec3 r    = q.Rotate(axis);
    EXPECT_TRUE(r.IsEqualApprox(axis, kEps));
}

TEST(Quat, FromEuler_Zero)
{
    const Quat q = Quat::FromEuler({ 0.f, 0.f, 0.f });
    EXPECT_TRUE(q.IsEqualApprox(Quat::kIdentity, kEps));
}

TEST(Quat, FromEuler_RoundTrip)
{
    // 짐벌락 회피: |pitch(x)| < π/2 범위 사용
    const Vec3 euler { 0.1f, 0.2f, -0.1f };
    const Quat q  = Quat::FromEuler(euler);
    const Vec3 e2 = q.ToEuler();
    const Quat q2 = Quat::FromEuler(e2);

    // 부호 반전된 동일 회전 처리 (q와 -q는 동일 회전)
    const float sign = q.Dot(q2) >= 0.f ? 1.f : -1.f;

    const Vec3 v0 { 1.f, 0.f, 0.f };
    const Vec3 v1 { 0.f, 1.f, 0.f };
    const Vec3 v2 { 1.f, 2.f, 3.f };
    EXPECT_TRUE(q.Rotate(v0).IsEqualApprox(q2.Rotate(v0), kLooseEps));
    EXPECT_TRUE(q.Rotate(v1).IsEqualApprox(q2.Rotate(v1), kLooseEps));
    EXPECT_TRUE(q.Rotate(v2).IsEqualApprox(q2.Rotate(v2), kLooseEps));

    // 쿼터니언 자체도 (부호 정규화 후) 동일해야 함
    const Quat q2s = q2 * sign;
    EXPECT_TRUE(q.IsEqualApprox(q2s, kLooseEps));
}
TEST(Quat, Conjugate)
{
    const Quat q  = { 1.f, 2.f, 3.f, 4.f };
    const Quat qc = q.Conjugate();
    EXPECT_FLOAT_EQ(qc.x, -1.f);
    EXPECT_FLOAT_EQ(qc.y, -2.f);
    EXPECT_FLOAT_EQ(qc.z, -3.f);
    EXPECT_FLOAT_EQ(qc.w, 4.f);
}

TEST(Quat, Inverse_UnitQuaternion)
{
    // 단위 쿼터니언의 역 == 켤레
    const Quat q    = Quat::FromAxisAngle(Vec3(1.f, 2.f, 3.f).Normalized(), 0.8f);
    const Quat qInv = q.Inverse();
    const Quat qCon = q.Conjugate();
    EXPECT_TRUE(qInv.IsEqualApprox(qCon, kEps));
}

TEST(Quat, Inverse_RotatesBack)
{
    const Quat q    = Quat::FromAxisAngle(Vec3(0.3f, 0.7f, 0.5f).Normalized(), 1.3f);
    const Quat qInv = q.Inverse();
    const Vec3 v { 1.f, 2.f, 3.f };
    const Vec3 r = qInv.Rotate(q.Rotate(v));
    EXPECT_TRUE(r.IsEqualApprox(v, kEps));
}

TEST(Quat, Normalize)
{
    Quat q { 2.f, 0.f, 0.f, 0.f };
    q.Normalize();
    EXPECT_TRUE(Approx(q.Length(), 1.f));
}

TEST(Quat, Dot)
{
    const Quat a { 1.f, 2.f, 3.f, 4.f };
    const Quat b { 5.f, 6.f, 7.f, 8.f };
    EXPECT_TRUE(Approx(a.Dot(b), 5.f + 12.f + 21.f + 32.f));
}

TEST(Quat, Multiply_Composition)
{
    // q1 * q2 means: apply q2 first, then q1 (per header note)
    // X축 90도 후 Y축 90도 적용: 조합 q = qX * qY  → v' = q.Rotate(v) = qX.Rotate(qY.Rotate(v))
    const Quat qX = Quat::FromAxisAngle(Vec3::kUnitX, kHalfPiF);
    const Quat qY = Quat::FromAxisAngle(Vec3::kUnitY, kHalfPiF);

    const Vec3 v { 1.f, 0.f, 0.f };
    const Vec3 sep = qX.Rotate(qY.Rotate(v));   // qY 먼저, 그 다음 qX

    const Quat q   = qX * qY;
    const Vec3 com = q.Rotate(v);
    EXPECT_TRUE(com.IsEqualApprox(sep, kEps));
}

TEST(Quat, MultiplyIdentity)
{
    const Quat q = Quat::FromAxisAngle(Vec3::kUnitY, 0.9f);
    EXPECT_TRUE((q * Quat::kIdentity).IsEqualApprox(q, kEps));
    EXPECT_TRUE((Quat::kIdentity * q).IsEqualApprox(q, kEps));
}

TEST(Quat, Lerp_Endpoints)
{
    const Quat a = Quat::kIdentity;
    const Quat b = Quat::FromAxisAngle(Vec3::kUnitY, 1.f);
    EXPECT_TRUE(a.Lerp(b, 0.f).IsEqualApprox(a.Normalized(), kEps));
    EXPECT_TRUE(a.Lerp(b, 1.f).IsEqualApprox(b.Normalized(), kEps));
}

TEST(Quat, Slerp_Endpoints)
{
    const Quat a = Quat::kIdentity;
    const Quat b = Quat::FromAxisAngle(Vec3::kUnitY, 1.f);
    EXPECT_TRUE(a.Slerp(b, 0.f).IsEqualApprox(a, kEps));
    EXPECT_TRUE(a.Slerp(b, 1.f).IsEqualApprox(b, kEps));
}

TEST(Quat, Slerp_HalfAngle)
{
    // 0도와 90도의 중간은 45도 회전이어야 함
    const Quat a     = Quat::kIdentity;
    const Quat b     = Quat::FromAxisAngle(Vec3::kUnitY, kHalfPiF);
    const Quat mid   = a.Slerp(b, 0.5f);
    const Quat exact = Quat::FromAxisAngle(Vec3::kUnitY, kQuarterPiF);
    EXPECT_TRUE(mid.IsEqualApprox(exact, kLooseEps));
}

TEST(Quat, Arithmetic)
{
    const Quat a { 1.f, 2.f, 3.f, 4.f };
    const Quat b { 5.f, 6.f, 7.f, 8.f };

    const Quat sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 6.f);
    EXPECT_FLOAT_EQ(sum.w, 12.f);

    const Quat sub = b - a;
    EXPECT_FLOAT_EQ(sub.x, 4.f);

    const Quat mul = a * 2.f;
    EXPECT_FLOAT_EQ(mul.x, 2.f);
    EXPECT_FLOAT_EQ(mul.w, 8.f);

    const Quat div = a / 2.f;
    EXPECT_FLOAT_EQ(div.x, 0.5f);

    const Quat neg = -a;
    EXPECT_FLOAT_EQ(neg.x, -1.f);
    EXPECT_FLOAT_EQ(neg.w, -4.f);
}

TEST(Quat, FromRotationMatrix_RoundTrip)
{
    const Quat q  = Quat::FromAxisAngle(Vec3(1.f, 1.f, 0.f).Normalized(), 1.2f);
    const Mtx4 m  = Mtx4::FromQuaternion(q);
    Quat       q2 = Quat::FromRotationMatrix(m);

    // 부호가 반대일 수 있음 (둘 다 동일 회전 표현)
    if (q.Dot(q2) < 0.f)
    {
        q2 = -q2;
    }
    EXPECT_TRUE(q.IsEqualApprox(q2, kLooseEps));
}

// =============================================================================
//  Mtx4 테스트 (왼손 좌표계 / 행 기준)
// =============================================================================

TEST(Mtx4, Identity)
{
    const Mtx4& m = Mtx4::kIdentity;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(m.m[i][j], (i == j) ? 1.f : 0.f));
        }
    }
}

TEST(Mtx4, Identity_DirectionMembers)
{
    // 행 기준: row 0 = right, row 1 = up, row 2 = forward, row 3 = translation
    const Mtx4& m = Mtx4::kIdentity;
    EXPECT_TRUE(m.r0.IsEqualApprox({ 1.f, 0.f, 0.f, 0.f }));
    EXPECT_TRUE(m.r1.IsEqualApprox({ 0.f, 1.f, 0.f, 0.f }));
    EXPECT_TRUE(m.r2.IsEqualApprox({ 0.f, 0.f, 1.f, 0.f }));
    EXPECT_TRUE(m.r3.IsEqualApprox({ 0.f, 0.f, 0.f, 1.f }));
}

TEST(Mtx4, Translation_RowMajor)
{
    // 행 기준: translation은 마지막 행 (row 3)
    const Mtx4 m = Mtx4::CreateTranslation({ 3.f, 4.f, 5.f });
    EXPECT_TRUE(Approx(m.m[3][0], 3.f));
    EXPECT_TRUE(Approx(m.m[3][1], 4.f));
    EXPECT_TRUE(Approx(m.m[3][2], 5.f));
    EXPECT_TRUE(Approx(m.m[3][3], 1.f));

    const Vec3 v { 1.f, 1.f, 1.f };
    const Vec3 r = m.Mult(v);
    EXPECT_TRUE(r.IsEqualApprox({ 4.f, 5.f, 6.f }, kEps));
}

TEST(Mtx4, Scale)
{
    const Mtx4 m = Mtx4::CreateScale({ 2.f, 3.f, 4.f });
    const Vec3 v { 1.f, 1.f, 1.f };
    const Vec3 r = m.Mult(v);
    EXPECT_TRUE(r.IsEqualApprox({ 2.f, 3.f, 4.f }, kEps));
}

// ─── 회전 행렬 레이아웃 검증 (행 기준, 왼손) ─────────────────────────────────
// RotX: m[1][2] = +s, m[2][1] = -s   (+y가 +z 방향으로)
// RotY: m[0][2] = -s, m[2][0] = +s   (+z가 +x 방향으로)
// RotZ: m[0][1] = +s, m[1][0] = -s   (+x가 +y 방향으로)

TEST(Mtx4, CreateRotationX_Layout)
{
    constexpr float a = 0.5f;
    const float     c = Jam::Cos(a);
    const float     s = Jam::Sin(a);
    const Mtx4      m = Mtx4::CreateRotationX(a);

    EXPECT_TRUE(Approx(m.m[0][0], 1.f));
    EXPECT_TRUE(Approx(m.m[1][1], c));
    EXPECT_TRUE(Approx(m.m[1][2], s));
    EXPECT_TRUE(Approx(m.m[2][1], -s));
    EXPECT_TRUE(Approx(m.m[2][2], c));
}

TEST(Mtx4, CreateRotationY_Layout)
{
    constexpr float a = 0.5f;
    const float     c = Jam::Cos(a);
    const float     s = Jam::Sin(a);
    const Mtx4      m = Mtx4::CreateRotationY(a);

    EXPECT_TRUE(Approx(m.m[0][0], c));
    EXPECT_TRUE(Approx(m.m[0][2], -s));
    EXPECT_TRUE(Approx(m.m[1][1], 1.f));
    EXPECT_TRUE(Approx(m.m[2][0], s));
    EXPECT_TRUE(Approx(m.m[2][2], c));
}

TEST(Mtx4, CreateRotationZ_Layout)
{
    constexpr float a = 0.5f;
    const float     c = Jam::Cos(a);
    const float     s = Jam::Sin(a);
    const Mtx4      m = Mtx4::CreateRotationZ(a);

    EXPECT_TRUE(Approx(m.m[0][0], c));
    EXPECT_TRUE(Approx(m.m[0][1], s));
    EXPECT_TRUE(Approx(m.m[1][0], -s));
    EXPECT_TRUE(Approx(m.m[1][1], c));
    EXPECT_TRUE(Approx(m.m[2][2], 1.f));
}

TEST(Mtx4, RotationX_Transform)
{
    // X축 +90도: +Y -> +Z
    const Mtx4 m = Mtx4::CreateRotationX(kHalfPiF);
    const Vec3 r = m.Mult(Vec3::kUnitY);
    EXPECT_TRUE(Approx(r.x, 0.f, kEps));
    EXPECT_TRUE(Approx(r.y, 0.f, kEps));
    EXPECT_TRUE(Approx(r.z, 1.f, kEps));
}

TEST(Mtx4, RotationY_Transform)
{
    // Y축 +90도: +Z -> +X
    const Mtx4 m = Mtx4::CreateRotationY(kHalfPiF);
    const Vec3 r = m.Mult(Vec3::kUnitZ);
    EXPECT_TRUE(Approx(r.x, 1.f, kEps));
    EXPECT_TRUE(Approx(r.y, 0.f, kEps));
    EXPECT_TRUE(Approx(r.z, 0.f, kEps));
}

TEST(Mtx4, RotationZ_Transform)
{
    // Z축 +90도: +X -> +Y
    const Mtx4 m = Mtx4::CreateRotationZ(kHalfPiF);
    const Vec3 r = m.Mult(Vec3::kUnitX);
    EXPECT_TRUE(Approx(r.x, 0.f, kEps));
    EXPECT_TRUE(Approx(r.y, 1.f, kEps));
    EXPECT_TRUE(Approx(r.z, 0.f, kEps));
}

TEST(Mtx4, Rotation_AxisInvariant)
{
    // 회전축 위의 벡터는 어떤 각도로 회전해도 불변
    EXPECT_TRUE(Mtx4::CreateRotationX(0.9f).Mult(Vec3::kUnitX).IsEqualApprox(Vec3::kUnitX, kEps));
    EXPECT_TRUE(Mtx4::CreateRotationY(0.9f).Mult(Vec3::kUnitY).IsEqualApprox(Vec3::kUnitY, kEps));
    EXPECT_TRUE(Mtx4::CreateRotationZ(0.9f).Mult(Vec3::kUnitZ).IsEqualApprox(Vec3::kUnitZ, kEps));
}

TEST(Mtx4, FromAxisAngle_MatchesRotationY)
{
    constexpr float angle = 0.7f;
    const Mtx4      a     = Mtx4::FromAxisAngle(Vec3::kUnitY, angle);
    const Mtx4      b     = Mtx4::CreateRotationY(angle);
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(a.m[i][j], b.m[i][j], kEps));
        }
    }
}

TEST(Mtx4, FromAxisAngle_MatchesQuaternion)
{
    const Vec3      axis  = Vec3(0.3f, 0.8f, 0.5f).Normalized();
    constexpr float angle = 1.1f;
    const Mtx4      ma    = Mtx4::FromAxisAngle(axis, angle);
    const Mtx4      mq    = Mtx4::FromQuaternion(Quat::FromAxisAngle(axis, angle));
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(ma.m[i][j], mq.m[i][j], kLooseEps));
        }
    }
}

TEST(Mtx4, Transpose)
{
    const Mtx4 m { 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f };
    const Mtx4 t = m.Transpose();
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(t.m[i][j], m.m[j][i]));
        }
    }
}

TEST(Mtx4, Transpose_Double)
{
    const Mtx4 m  = Mtx4::FromAxisAngle(Vec3::kUnitY, 0.7f);
    const Mtx4 mt = m.Transpose().Transpose();
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(mt.m[i][j], m.m[i][j], kEps));
        }
    }
}

TEST(Mtx4, Inverse_Identity)
{
    const Mtx4 inv = Mtx4::kIdentity.Inverse();
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(inv.m[i][j], (i == j) ? 1.f : 0.f));
        }
    }
}

TEST(Mtx4, Inverse_Rotation_EqualsTranspose)
{
    // 순수 회전 행렬의 역행렬 = 전치
    const Mtx4 m   = Mtx4::CreateRotationY(0.7f);
    const Mtx4 inv = m.Inverse();
    const Mtx4 t   = m.Transpose();
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(inv.m[i][j], t.m[i][j], kLooseEps));
        }
    }
}

TEST(Mtx4, Inverse_ProductIsIdentity)
{
    const Mtx4 m = Mtx4::CreateSRT(
        { 2.f, 3.f, 1.5f },
        Quat::FromAxisAngle({ 0.3f, 0.7f, 0.6f }, 0.5f),
        { 10.f, -5.f, 3.f });
    const Mtx4 inv = m.Inverse();
    const Mtx4 r   = m * inv;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(r.m[i][j], (i == j) ? 1.f : 0.f, kLooseEps));
        }
    }
}

TEST(Mtx4, Multiply_Identity)
{
    const Mtx4 m = Mtx4::CreateRotationZ(0.4f);
    const Mtx4 r = m * Mtx4::kIdentity;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(r.m[i][j], m.m[i][j], kEps));
        }
    }
}

TEST(Mtx4, Multiply_Order_RowMajor)
{
    // 행 기준: v * (A * B) == (v * A) * B (먼저 A 적용, 그 다음 B 적용)
    const Mtx4 A = Mtx4::CreateRotationZ(kHalfPiF);   // +X -> +Y
    const Mtx4 B = Mtx4::CreateTranslation({ 10.f, 0.f, 0.f });
    const Vec3 v { 1.f, 0.f, 0.f };

    const Vec3 r1 = (A * B).Mult(v);
    const Vec3 r2 = B.Mult(A.Mult(v));
    EXPECT_TRUE(r1.IsEqualApprox(r2, kEps));

    // A(회전)가 먼저: (1,0,0) -> (0,1,0), 그 다음 B(평행이동): (10,1,0)
    EXPECT_TRUE(r1.IsEqualApprox({ 10.f, 1.f, 0.f }, kEps));
}

TEST(Mtx4, AddSubtract)
{
    const Mtx4 a = Mtx4::CreateScale({ 3.f, 3.f, 3.f });
    const Mtx4 b = Mtx4::CreateScale({ 1.f, 1.f, 1.f });   // Identity
    const Mtx4 r = a - b;
    EXPECT_TRUE(Approx(r.m[0][0], 2.f));
    EXPECT_TRUE(Approx(r.m[1][1], 2.f));
    EXPECT_TRUE(Approx(r.m[2][2], 2.f));
    EXPECT_TRUE(Approx(r.m[3][3], 0.f));
}

TEST(Mtx4, ScalarMultiply)
{
    const Mtx4 r = Mtx4::kIdentity * 5.f;
    EXPECT_FLOAT_EQ(r.m[0][0], 5.f);
    EXPECT_FLOAT_EQ(r.m[1][1], 5.f);
    EXPECT_FLOAT_EQ(r.m[2][2], 5.f);
    EXPECT_FLOAT_EQ(r.m[3][3], 5.f);
}

TEST(Mtx4, CompoundAssignments)
{
    Mtx4       a = Mtx4::kIdentity;
    const Mtx4 b = Mtx4::CreateScale({ 2.f, 2.f, 2.f });
    a += b;
    EXPECT_TRUE(Approx(a.m[0][0], 3.f));

    a -= b;
    EXPECT_TRUE(Approx(a.m[0][0], 1.f));

    a *= b;
    EXPECT_TRUE(Approx(a.m[0][0], 2.f));

    a *= 2.f;
    EXPECT_TRUE(Approx(a.m[0][0], 4.f));
}

TEST(Mtx4, CreateSRT_TranslationRow)
{
    const Vec3 scale { 2.f, 3.f, 1.f };
    const Quat rot = Quat::kIdentity;
    const Vec3 trans { 5.f, 7.f, -3.f };
    const Mtx4 m = Mtx4::CreateSRT(scale, rot, trans);

    EXPECT_TRUE(Approx(m.m[3][0], trans.x));
    EXPECT_TRUE(Approx(m.m[3][1], trans.y));
    EXPECT_TRUE(Approx(m.m[3][2], trans.z));
    EXPECT_TRUE(Approx(m.m[3][3], 1.f));

    // 원점 (0,0,0,1) 변환 시 translation이 그대로 나와야 함
    const Vec3 t = m.Mult(Vec3::kZero);
    EXPECT_TRUE(t.IsEqualApprox(trans, kEps));
}

TEST(Mtx4, CreateSRT_EquivalentToProduct)
{
    const Vec3 scale { 2.f, 1.f, 3.f };
    const Quat rot = Quat::FromAxisAngle(Vec3::kUnitY, 0.5f);
    const Vec3 trans { 5.f, -3.f, 7.f };

    const Mtx4 combined = Mtx4::CreateSRT(scale, rot, trans);
    const Mtx4 separate = Mtx4::CreateScale(scale)
                        * Mtx4::FromQuaternion(rot)
                        * Mtx4::CreateTranslation(trans);

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(combined.m[i][j], separate.m[i][j], kLooseEps))
                << "m[" << i << "][" << j << "]";
        }
    }
}

TEST(Mtx4, DecomposeSRT)
{
    const Vec3 scale { 2.f, 3.f, 1.5f };
    const Quat rot = Quat::FromAxisAngle(Vec3(0.3f, 0.8f, 0.5f).Normalized(), 0.7f);
    const Vec3 trans { 10.f, -5.f, 3.f };

    const Mtx4 m = Mtx4::CreateSRT(scale, rot, trans);

    Vec3 s2 = {};
    Quat r2 = {};
    Vec3 t2 = {};
    m.DecomposeSRT(&s2, &r2, &t2);

    EXPECT_TRUE(s2.IsEqualApprox(scale, kLooseEps));
    EXPECT_TRUE(t2.IsEqualApprox(trans, kEps));
    if (r2.Dot(rot) < 0.f) r2 = -r2;
    EXPECT_TRUE(r2.IsEqualApprox(rot, kLooseEps));
}

TEST(Mtx4, DecomposeSRT_Nulls)
{
    const Mtx4 m = Mtx4::CreateTranslation({ 1.f, 2.f, 3.f });
    // null 인자 전달도 크래시 없이 동작해야 함
    Vec3 t = {};
    m.DecomposeSRT(nullptr, nullptr, &t);
    EXPECT_TRUE(t.IsEqualApprox({ 1.f, 2.f, 3.f }, kEps));
}

// ─── 투영 행렬 (DirectX 스타일 NDC: z in [0, 1]) ──────────────────────────

TEST(Mtx4, PerspectiveFov_NearPlaneMapsToZero)
{
    constexpr float fov    = kHalfPiF;
    constexpr float aspect = 16.f / 9.f;
    constexpr float nZ     = 0.1f;
    constexpr float fZ     = 100.f;
    const Mtx4      p      = Mtx4::CreatePerspectiveFov(fov, aspect, nZ, fZ);

    const Vec4  nPt { 0.f, 0.f, nZ, 1.f };
    const Vec4  r    = p.Mult(nPt);
    const float ndcZ = (r.w != 0.f) ? r.z / r.w : r.z;
    EXPECT_TRUE(Approx(ndcZ, 0.f, kLooseEps));
}

TEST(Mtx4, PerspectiveFov_FarPlaneMapsToOne)
{
    constexpr float fov    = kHalfPiF;
    constexpr float aspect = 16.f / 9.f;
    constexpr float nZ     = 0.1f;
    constexpr float fZ     = 100.f;
    const Mtx4      p      = Mtx4::CreatePerspectiveFov(fov, aspect, nZ, fZ);

    const Vec4  fPt { 0.f, 0.f, fZ, 1.f };
    const Vec4  r    = p.Mult(fPt);
    const float ndcZ = (r.w != 0.f) ? r.z / r.w : r.z;
    EXPECT_TRUE(Approx(ndcZ, 1.f, kLooseEps));
}

TEST(Mtx4, Ortho_Center)
{
    constexpr float w = 800.f, h = 600.f, n = 0.f, f = 1000.f;
    const Mtx4      p = Mtx4::CreateOrtho(w, h, n, f);

    // 화면 중심(x=0, y=0) -> NDC (0, 0)
    const Vec4 center { 0.f, 0.f, 0.f, 1.f };
    const Vec4 r = p.Mult(center);
    EXPECT_TRUE(Approx(r.x, 0.f, kEps));
    EXPECT_TRUE(Approx(r.y, 0.f, kEps));
}

TEST(Mtx4, Ortho_NearFarMapping)
{
    constexpr float w = 800.f, h = 600.f, n = 0.f, f = 100.f;
    const Mtx4      p = Mtx4::CreateOrtho(w, h, n, f);

    // near -> 0, far -> 1
    const Vec4 pn { 0.f, 0.f, n, 1.f };
    const Vec4 pf { 0.f, 0.f, f, 1.f };
    EXPECT_TRUE(Approx(p.Mult(pn).z, 0.f, kEps));
    EXPECT_TRUE(Approx(p.Mult(pf).z, 1.f, kEps));
}

TEST(Mtx4, LookAt_ForwardTowardsTarget)
{
    // 왼손 좌표계: eye에서 target을 바라보는 뷰 행렬
    const Vec3 eye { 0.f, 0.f, -5.f };
    const Vec3 target { 0.f, 0.f, 0.f };
    const Vec3 up { 0.f, 1.f, 0.f };
    const Mtx4 view = Mtx4::CreateLookAt(eye, target, up);

    // eye 자체를 변환하면 원점이 되어야 함
    const Vec3 transformedEye = view.Mult(eye);
    EXPECT_TRUE(transformedEye.IsEqualApprox(Vec3::kZero, kLooseEps));
}

TEST(Mtx4, LookAt_TargetOnForwardAxis)
{
    // eye에서 보았을 때 target은 뷰 공간에서 +Z 방향 (왼손, forward = +Z)
    const Vec3 eye { 0.f, 0.f, -5.f };
    const Vec3 target { 0.f, 0.f, 0.f };
    const Mtx4 view = Mtx4::CreateLookAt(eye, target, Vec3::kUnitY);

    const Vec3 viewTarget = view.Mult(target);
    EXPECT_TRUE(Approx(viewTarget.x, 0.f, kLooseEps));
    EXPECT_TRUE(Approx(viewTarget.y, 0.f, kLooseEps));
    EXPECT_GT(viewTarget.z, 0.f);   // +Z (왼손)
}

// ─── Vec * Mtx 변환 ────────────────────────────────────────────────────────

TEST(Mtx4, Vec3Transform_PerspectiveDivide)
{
    // Vec3 * Mtx4는 w로 나눠 퍼스펙티브 디바이드 수행
    const Mtx4 s = Mtx4::CreateScale({ 2.f, 2.f, 2.f });
    const Mtx4 t = Mtx4::CreateTranslation({ 10.f, 0.f, 0.f });
    const Mtx4 m = s * t;

    const Vec3 v { 1.f, 0.f, 0.f };
    const Vec3 r = m.Mult(v);
    // 스케일 먼저(2), 그 다음 이동(+10) → 12
    EXPECT_TRUE(Approx(r.x, 12.f, kEps));
    EXPECT_TRUE(Approx(r.y, 0.f, kEps));
    EXPECT_TRUE(Approx(r.z, 0.f, kEps));
}

TEST(Mtx4, Vec4Transform_NoPerspectiveDivide)
{
    // Vec4 변환은 w로 나누지 않음 (동차좌표 그대로 반환)
    const Mtx4 t = Mtx4::CreateTranslation({ 1.f, 2.f, 3.f });
    const Vec4 v { 5.f, 5.f, 5.f, 1.f };
    const Vec4 r = t.Mult(v);
    EXPECT_TRUE(Approx(r.x, 6.f));
    EXPECT_TRUE(Approx(r.y, 7.f));
    EXPECT_TRUE(Approx(r.z, 8.f));
    EXPECT_TRUE(Approx(r.w, 1.f));
}

TEST(Mtx4, Vec3TransformAssign)
{
    Vec3 v { 1.f, 2.f, 3.f };
    v = Mtx4::CreateTranslation({ 10.f, 20.f, 30.f }).Mult(v);
    EXPECT_TRUE(v.IsEqualApprox({ 11.f, 22.f, 33.f }, kEps));
}

// =============================================================================
//  교차 검증: Quat.Rotate <-> Mtx4 변환
// =============================================================================

TEST(Cross, QuatRotate_MatchesMatrix)
{
    const Vec3      axis  = Vec3(1.f, 1.f, 0.f).Normalized();
    constexpr float angle = 1.1f;

    const Quat q = Quat::FromAxisAngle(axis, angle);
    const Mtx4 m = Mtx4::FromQuaternion(q);

    const Vec3 v { 1.f, 2.f, 3.f };
    const Vec3 rq = q.Rotate(v);
    const Vec3 rm = m.Mult(v);

    EXPECT_TRUE(rq.IsEqualApprox(rm, kLooseEps));
}

TEST(Cross, QuatComposition_MatchesMatrixMultiply)
{
    const Quat qA = Quat::FromAxisAngle(Vec3::kUnitX, 0.4f);
    const Quat qB = Quat::FromAxisAngle(Vec3::kUnitY, 0.8f);

    const Mtx4 mA = Mtx4::FromQuaternion(qA);
    const Mtx4 mB = Mtx4::FromQuaternion(qB);

    // 행 기준 행렬: mA * mB === (먼저 A 적용 후 B 적용)
    // 쿼터니언 qA * qB === (먼저 B 적용 후 A 적용) ∴ 순서가 반대
    // 따라서 "먼저 A 후 B" = 쿼터니언으로는 qB * qA
    const Quat qComposed = qB * qA;
    const Mtx4 mComposed = mA * mB;

    const Vec3 v { 1.f, 2.f, 3.f };
    const Vec3 rq = qComposed.Rotate(v);
    const Vec3 rm = mComposed.Mult(v);
    EXPECT_TRUE(rq.IsEqualApprox(rm, kLooseEps));
}

// =============================================================================
//  Transform 클래스 테스트
// =============================================================================

TEST(Transform, DefaultState)
{
    const Transform t;
    EXPECT_TRUE(t.GetScale().IsEqualApprox({ 1.f, 1.f, 1.f }));
    EXPECT_TRUE(t.GetTranslation().IsZeroApprox());
    EXPECT_TRUE(t.GetRotation().IsEqualApprox(Quat::kIdentity, kEps));
}

TEST(Transform, SetGetTranslation)
{
    Transform t;
    t.SetTranslation({ 1.f, 2.f, 3.f });
    EXPECT_TRUE(t.GetTranslation().IsEqualApprox({ 1.f, 2.f, 3.f }));
    EXPECT_FLOAT_EQ(t.GetTranslationX(), 1.f);
    EXPECT_FLOAT_EQ(t.GetTranslationY(), 2.f);
    EXPECT_FLOAT_EQ(t.GetTranslationZ(), 3.f);

    t.SetTranslationX(10.f);
    t.SetTranslationY(20.f);
    t.SetTranslationZ(30.f);
    EXPECT_TRUE(t.GetTranslation().IsEqualApprox({ 10.f, 20.f, 30.f }));
}

TEST(Transform, AddTranslation)
{
    Transform t;
    t.SetTranslation({ 1.f, 2.f, 3.f });
    t.AddTranslation({ 1.f, 1.f, 1.f });
    EXPECT_TRUE(t.GetTranslation().IsEqualApprox({ 2.f, 3.f, 4.f }));

    t.AddTranslationX(10.f);
    EXPECT_FLOAT_EQ(t.GetTranslationX(), 12.f);
}

TEST(Transform, SetGetScale)
{
    Transform t;
    t.SetScale({ 2.f, 3.f, 4.f });
    EXPECT_TRUE(t.GetScale().IsEqualApprox({ 2.f, 3.f, 4.f }));

    t.SetScale(5.f);
    EXPECT_TRUE(t.GetScale().IsEqualApprox({ 5.f, 5.f, 5.f }));

    t.SetScale(1.f, 2.f, 3.f);
    EXPECT_FLOAT_EQ(t.GetScaleX(), 1.f);
    EXPECT_FLOAT_EQ(t.GetScaleY(), 2.f);
    EXPECT_FLOAT_EQ(t.GetScaleZ(), 3.f);
}

TEST(Transform, AddScale)
{
    Transform t;
    t.SetScale(1.f);
    t.AddScale({ 1.f, 2.f, 3.f });
    EXPECT_TRUE(t.GetScale().IsEqualApprox({ 2.f, 3.f, 4.f }));
}

TEST(Transform, SetRotation_Quaternion)
{
    Transform  t;
    const Quat q = Quat::FromAxisAngle(Vec3::kUnitY, kHalfPiF);
    t.SetRotation(q);

    // Forward(+Z)를 회전한 결과는 +X가 되어야 함
    EXPECT_TRUE(t.GetForward().IsEqualApprox(Vec3::kUnitX, kLooseEps));
}

TEST(Transform, SetRotation_AxisAngle)
{
    Transform t;
    t.SetRotation(Vec3::kUnitZ, kHalfPiF);

    // Z축 +90도: Right(+X) -> +Y
    EXPECT_TRUE(t.GetRight().IsEqualApprox(Vec3::kUnitY, kLooseEps));
}

TEST(Transform, DirectionAxes_Identity)
{
    const Transform t;
    EXPECT_TRUE(t.GetRight().IsEqualApprox(Vec3::kUnitX, kEps));
    EXPECT_TRUE(t.GetUp().IsEqualApprox(Vec3::kUnitY, kEps));
    EXPECT_TRUE(t.GetForward().IsEqualApprox(Vec3::kUnitZ, kEps));
    EXPECT_TRUE(t.GetLeft().IsEqualApprox(-Vec3::kUnitX, kEps));
    EXPECT_TRUE(t.GetDown().IsEqualApprox(-Vec3::kUnitY, kEps));
    EXPECT_TRUE(t.GetBackward().IsEqualApprox(-Vec3::kUnitZ, kEps));
}

TEST(Transform, DirectionAxes_AfterYRotation90)
{
    Transform t;
    t.SetRotation(Vec3::kUnitY, kHalfPiF);

    // Y축 +90도: Forward(+Z) -> +X, Right(+X) -> -Z
    EXPECT_TRUE(t.GetForward().IsEqualApprox(Vec3::kUnitX, kLooseEps));
    EXPECT_TRUE(t.GetRight().IsEqualApprox(-Vec3::kUnitZ, kLooseEps));
    EXPECT_TRUE(t.GetUp().IsEqualApprox(Vec3::kUnitY, kLooseEps));   // Y축 불변
}

TEST(Transform, AddRotation)
{
    Transform t;
    t.AddRotation(Vec3::kUnitY, kQuarterPiF);   // 45도
    t.AddRotation(Vec3::kUnitY, kQuarterPiF);   // 추가 45도 -> 총 90도

    EXPECT_TRUE(t.GetForward().IsEqualApprox(Vec3::kUnitX, kLooseEps));
}

TEST(Transform, AddRotationPitchYawRoll)
{
    Transform t1;
    t1.AddRotationYaw(kHalfPiF);
    Transform t2;
    t2.AddRotation(0.f, kHalfPiF, 0.f);

    const Vec3 v { 1.f, 2.f, 3.f };
    // 동일 회전이어야 함
    const Vec3 r1 = t1.GetRotation().Rotate(v);
    const Vec3 r2 = t2.GetRotation().Rotate(v);
    EXPECT_TRUE(r1.IsEqualApprox(r2, kLooseEps));
}

TEST(Transform, DirtyFlag)
{
    Transform t;
    (void)t.GetSRT();   // 캐시 생성
    EXPECT_FALSE(t.IsDirty());

    t.SetTranslation({ 1.f, 0.f, 0.f });
    EXPECT_TRUE(t.IsDirty());

    (void)t.GetSRT();
    EXPECT_FALSE(t.IsDirty());

    t.SetScale(2.f);
    EXPECT_TRUE(t.IsDirty());
}

TEST(Transform, GetSRT_MatchesCreateSRT)
{
    Transform t;
    t.SetScale({ 2.f, 3.f, 1.5f });
    t.SetRotation(Quat::FromAxisAngle(Vec3::kUnitY, 0.7f));
    t.SetTranslation({ 10.f, -5.f, 3.f });

    const Mtx4& got      = t.GetSRT();
    const Mtx4  expected = Mtx4::CreateSRT(
        { 2.f, 3.f, 1.5f },
        Quat::FromAxisAngle(Vec3::kUnitY, 0.7f),
        { 10.f, -5.f, 3.f });

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(got.m[i][j], expected.m[i][j], kLooseEps));
        }
    }
}

TEST(Transform, SetSRT_RoundTrip)
{
    const Vec3 scale { 2.f, 3.f, 1.5f };
    const Quat rot = Quat::FromAxisAngle(Vec3(0.3f, 0.8f, 0.5f).Normalized(), 0.6f);
    const Vec3 trans { 10.f, -5.f, 3.f };

    const Mtx4 original = Mtx4::CreateSRT(scale, rot, trans);

    Transform t;
    t.SetSRT(original);

    EXPECT_TRUE(t.GetScale().IsEqualApprox(scale, kLooseEps));
    EXPECT_TRUE(t.GetTranslation().IsEqualApprox(trans, kEps));

    Quat rot2 = t.GetRotation();
    if (rot2.Dot(rot) < 0.f) rot2 = -rot2;
    EXPECT_TRUE(rot2.IsEqualApprox(rot, kLooseEps));
}

TEST(Transform, CalcInvSRT_InvertsSRT)
{
    Transform t;
    t.SetScale({ 2.f, 3.f, 1.5f });
    t.SetRotation(Quat::FromAxisAngle(Vec3(0.3f, 0.7f, 0.6f).Normalized(), 0.5f));
    t.SetTranslation({ 10.f, -5.f, 3.f });

    const Mtx4& srt    = t.GetSRT();
    const Mtx4  invSrt = t.CalcInvSRT();

    const Mtx4 product = srt * invSrt;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_TRUE(Approx(product.m[i][j], (i == j) ? 1.f : 0.f, kLooseEps));
        }
    }
}

TEST(Transform, Inversed_RoundTrip)
{
    Transform t;
    t.SetScale({ 2.f, 2.f, 2.f });
    t.SetRotation(Quat::FromAxisAngle(Vec3::kUnitY, 0.6f));
    t.SetTranslation({ 5.f, 3.f, -2.f });

    const Transform inv     = t.Inverse();
    const Transform doubleT = inv.Inverse();

    EXPECT_TRUE(doubleT.GetScale().IsEqualApprox(t.GetScale(), kLooseEps));
    EXPECT_TRUE(doubleT.GetTranslation().IsEqualApprox(t.GetTranslation(), kLooseEps));
}

TEST(Transform, Mult_TranslationOnly)
{
    Transform a;
    a.SetTranslation({ 1.f, 2.f, 3.f });

    Transform b;
    b.SetTranslation({ 10.f, 20.f, 30.f });

    // 스케일 1, 회전 단위 -> translation = 1+10, 2+20, 3+30
    EXPECT_TRUE(a.Mult(b).GetTranslation().IsEqualApprox({ 11.f, 22.f, 33.f }, kLooseEps));
}

TEST(Transform, ToAxisAngle_Identity)
{
    const Transform t;
    const AxisAngle aa = t.ToAxisAngle();
    // 단위 쿼터니언: 각도 0
    EXPECT_TRUE(Approx(aa.rad, 0.f, kEps));
}

TEST(Transform, ToAxisAngle_RoundTrip)
{
    const Vec3      axis  = Vec3(0.3f, 0.7f, 0.6f).Normalized();
    constexpr float angle = 1.1f;

    Transform t;
    t.SetRotation(axis, angle);
    const AxisAngle aa = t.ToAxisAngle();

    EXPECT_TRUE(Approx(aa.rad, angle, kLooseEps));
    EXPECT_TRUE(aa.axis.IsEqualApprox(axis, kLooseEps));
}

TEST(Transform, SetLookTo_ForwardMatches)
{
    Transform t;
    t.SetTranslation({ 0.f, 0.f, 0.f });
    t.SetLookTo(Vec3::kUnitX, Vec3::kUnitY);

    // 정면(+Z)이 +X 방향을 보게 됨
    EXPECT_TRUE(t.GetForward().IsEqualApprox(Vec3::kUnitX, kLooseEps));
}

TEST(Transform, SetLookAt_ForwardTowardsTarget)
{
    Transform t;
    t.SetTranslation({ 0.f, 0.f, 0.f });
    t.SetLookAt({ 10.f, 0.f, 0.f }, Vec3::kUnitY);

    EXPECT_TRUE(t.GetForward().IsEqualApprox(Vec3::kUnitX, kLooseEps));
}

// =============================================================================
//  Math 유틸리티 함수 테스트
// =============================================================================

TEST(Math, MinMaxClamp)
{
    EXPECT_EQ(Jam::Min(3, 5), 3);
    EXPECT_EQ(Jam::Max(3, 5), 5);
    EXPECT_EQ(Jam::Min(1, 2, 3, 4), 1);
    EXPECT_EQ(Jam::Max(1, 2, 3, 4), 4);
    EXPECT_EQ(Jam::Clamp(10, 0, 5), 5);
    EXPECT_EQ(Jam::Clamp(-5, 0, 5), 0);
    EXPECT_EQ(Jam::Clamp(3, 0, 5), 3);
}

TEST(Math, PowerOf2)
{
    EXPECT_TRUE(Jam::IsPowerOf2(1));
    EXPECT_TRUE(Jam::IsPowerOf2(2));
    EXPECT_TRUE(Jam::IsPowerOf2(1024));
    EXPECT_FALSE(Jam::IsPowerOf2(0));
    EXPECT_FALSE(Jam::IsPowerOf2(3));
    EXPECT_FALSE(Jam::IsPowerOf2(1023));

    EXPECT_EQ(Jam::NextPowerOf2(0), 1);
    EXPECT_EQ(Jam::NextPowerOf2(1), 1);
    EXPECT_EQ(Jam::NextPowerOf2(3), 4);
    EXPECT_EQ(Jam::NextPowerOf2(1000), 1024);
}

TEST(Math, DegRad)
{
    EXPECT_TRUE(Approx(Jam::ToRad(180.f), kPiF));
    EXPECT_TRUE(Approx(Jam::ToDeg(kPiF), 180.f));
    EXPECT_TRUE(Approx(Jam::ToRad(90.f), kHalfPiF));
}

TEST(Math, AbsSign)
{
    EXPECT_TRUE(Approx(Jam::Abs(-3.14f), 3.14f));
    EXPECT_TRUE(Approx(Jam::Abs(3.14f), 3.14f));

    EXPECT_FLOAT_EQ(Jam::Sign(5.f), 1.f);
    EXPECT_FLOAT_EQ(Jam::Sign(-5.f), -1.f);
    EXPECT_FLOAT_EQ(Jam::Sign(0.f), 0.f);
}

TEST(Math, FloorCeilRoundTrunc)
{
    EXPECT_TRUE(Approx(Jam::Floor(3.7f), 3.f));
    EXPECT_TRUE(Approx(Jam::Floor(-3.3f), -4.f));
    EXPECT_TRUE(Approx(Jam::Ceil(3.3f), 4.f));
    EXPECT_TRUE(Approx(Jam::Ceil(-3.7f), -3.f));
    EXPECT_TRUE(Approx(Jam::Round(3.5f), 4.f));
    EXPECT_TRUE(Approx(Jam::Round(3.4f), 3.f));
    EXPECT_TRUE(Approx(Jam::Trunc(3.7f), 3.f));
    EXPECT_TRUE(Approx(Jam::Trunc(-3.7f), -3.f));
    EXPECT_TRUE(Approx(Jam::Fract(3.25f), 0.25f));
}

TEST(Math, Lerp)
{
    EXPECT_TRUE(Approx(Jam::Lerp(0.f, 10.f, 0.5f), 5.f));
    EXPECT_TRUE(Approx(Jam::Lerp(10.f, 20.f, 0.f), 10.f));
    EXPECT_TRUE(Approx(Jam::Lerp(10.f, 20.f, 1.f), 20.f));
}

TEST(Math, Mod)
{
    EXPECT_TRUE(Approx(Jam::Mod(7.5f, 2.f), 1.5f));
    EXPECT_TRUE(Approx(Jam::Mod(-0.5f, 2.f), 1.5f));
}

TEST(Math, Wrap)
{
    EXPECT_TRUE(Approx(Jam::Wrap(370.f, 0.f, 360.f), 10.f));
    EXPECT_TRUE(Approx(Jam::Wrap(-10.f, 0.f, 360.f), 350.f));
    EXPECT_TRUE(Approx(Jam::Wrap(180.f, 0.f, 360.f), 180.f));
}

TEST(Math, IsZeroApprox)
{
    EXPECT_TRUE(Jam::IsZeroApprox(0.f));
    EXPECT_TRUE(Jam::IsZeroApprox(1e-8f));
    EXPECT_FALSE(Jam::IsZeroApprox(1.f));
}

TEST(Math, Sqrt_RSqrt)
{
    EXPECT_TRUE(Approx(Jam::Sqrt(4.f), 2.f));
    EXPECT_TRUE(Approx(Jam::Sqrt(9.f), 3.f));
    EXPECT_TRUE(Approx(Jam::RSqrt(4.f), 0.5f));
    EXPECT_TRUE(Approx(Jam::RSqrt(1.f), 1.f));
}

TEST(Math, Trig)
{
    EXPECT_TRUE(Approx(Jam::Sin(0.f), 0.f));
    EXPECT_TRUE(Approx(Jam::Sin(kHalfPiF), 1.f));
    EXPECT_TRUE(Approx(Jam::Cos(0.f), 1.f));
    EXPECT_TRUE(Approx(Jam::Cos(kHalfPiF), 0.f, kEps));
    EXPECT_TRUE(Approx(Jam::Asin(1.f), kHalfPiF));
    EXPECT_TRUE(Approx(Jam::Acos(0.f), kHalfPiF));
    EXPECT_TRUE(Approx(Jam::Atan2(1.f, 0.f), kHalfPiF));
}

TEST(Math, Limits)
{
    EXPECT_EQ(Limits<Int32>::kMin, INT32_MIN);
    EXPECT_EQ(Limits<Int32>::kMax, INT32_MAX);
    EXPECT_EQ(Limits<UInt32>::kMin, 0u);
    EXPECT_LT(Limits<float>::kLowest, 0.f);
    EXPECT_GT(Limits<float>::kHighest, 0.f);
}

// 간단한 디버그 테스트
TEST(Quat, Debug_SimpleMultiply)
{
    // 항등원 테스트
    const Quat qId = Quat::kIdentity;   // [0, 0, 0, 1]
    const Quat q1(1, 0, 0, 0);          // [1, 0, 0, 0]

    // qId * q1 should be q1
    Quat result1 = qId * q1;
    printf("qId * q1 = [%f, %f, %f, %f]\n", result1.x, result1.y, result1.z, result1.w);
    printf("expected  = [%f, %f, %f, %f]\n", q1.x, q1.y, q1.z, q1.w);
    EXPECT_TRUE(result1.IsEqualApprox(q1, kEps));

    // q1 * qId should be q1
    Quat result2 = q1 * qId;
    printf("q1 * qId = [%f, %f, %f, %f]\n", result2.x, result2.y, result2.z, result2.w);
    printf("expected  = [%f, %f, %f, %f]\n", q1.x, q1.y, q1.z, q1.w);
    EXPECT_TRUE(result2.IsEqualApprox(q1, kEps));
}

TEST(Quat, Debug_BasicMultiply)
{
    // 간단한 곱셈: i * i = -1
    // q1 = [1, 0, 0, 0] (i 단위)
    // q2 = [1, 0, 0, 0] (i 단위)
    // q1 * q2 = -1 = [0, 0, 0, -1]

    const Quat qi(1, 0, 0, 0);
    Quat       result = qi * qi;

    printf("i * i = [%f, %f, %f, %f]\n", result.x, result.y, result.z, result.w);
    printf("expected = [0, 0, 0, -1]\n");

    // 스칼라 방식으로 검증
    // xx = w1*x2 + x1*w2 + y1*z2 - z1*y2 = 0*1 + 1*0 + 0*0 - 0*0 = 0 ✓
    // yy = w1*y2 - x1*z2 + y1*w2 + z1*x2 = 0*0 - 1*0 + 0*0 + 0*1 = 0 ✓
    // zz = w1*z2 + x1*y2 - y1*x2 + z1*w2 = 0*0 + 1*0 - 0*1 + 0*0 = 0 ✓
    // ww = w1*w2 - x1*x2 - y1*y2 - z1*z2 = 0*0 - 1*1 - 0*0 - 0*0 = -1 ✓

    EXPECT_FLOAT_EQ(result.x, 0.f);
    EXPECT_FLOAT_EQ(result.y, 0.f);
    EXPECT_FLOAT_EQ(result.z, 0.f);
    EXPECT_FLOAT_EQ(result.w, -1.f);
}

TEST(Quat, Debug_AxisAngle)
{
    // 90도 회전 쿼터니언 확인
    const Quat qX = Quat::FromAxisAngle(Vec3::kUnitX, kHalfPiF);
    const Quat qY = Quat::FromAxisAngle(Vec3::kUnitY, kHalfPiF);

    printf("qX (X축 90도) = [%f, %f, %f, %f]\n", qX.x, qX.y, qX.z, qX.w);
    printf("qY (Y축 90도) = [%f, %f, %f, %f]\n", qY.x, qY.y, qY.z, qY.w);

    // 90도 회전 = sin(45도) * axis + cos(45도) * w
    // sin(45도) ≈ 0.7071, cos(45도) ≈ 0.7071
    // 따라서 qX ≈ [0.7071, 0, 0, 0.7071]
    // 따라서 qY ≈ [0, 0.7071, 0, 0.7071]
}

TEST(Quat, Debug_ScalarVsSIMD)
{
    const Quat qX = Quat::FromAxisAngle(Vec3::kUnitX, kHalfPiF);
    const Quat qY = Quat::FromAxisAngle(Vec3::kUnitY, kHalfPiF);

    // 스칼라 버전만 사용하도록 임시 코드
    const float xx = qX.w * qY.x + qX.x * qY.w + qX.y * qY.z - qX.z * qY.y;
    const float yy = qX.w * qY.y - qX.x * qY.z + qX.y * qY.w + qX.z * qY.x;
    const float zz = qX.w * qY.z + qX.x * qY.y - qX.y * qY.x + qX.z * qY.w;
    const float ww = qX.w * qY.w - qX.x * qY.x - qX.y * qY.y - qX.z * qY.z;

    printf("scalar result = [%f, %f, %f, %f]\n", xx, yy, zz, ww);

    // SIMD 버전
    const Quat q_simd = qX * qY;
    printf("SIMD result   = [%f, %f, %f, %f]\n", q_simd.x, q_simd.y, q_simd.z, q_simd.w);

    EXPECT_FLOAT_EQ(q_simd.x, xx);
    EXPECT_FLOAT_EQ(q_simd.y, yy);
    EXPECT_FLOAT_EQ(q_simd.z, zz);
    EXPECT_FLOAT_EQ(q_simd.w, ww);
}

TEST(Quat, Debug_M128Layout)
{
    const Quat q(1, 2, 3, 4);   // x=1, y=2, z=3, w=4

    float vals[4];
    _mm_storeu_ps(vals, q.m128);

    printf("m128 layout check:\n");
    printf("Expected: [1, 2, 3, 4] (x, y, z, w)\n");
    printf("Actual:   [%.1f, %.1f, %.1f, %.1f]\n", vals[0], vals[1], vals[2], vals[3]);

    EXPECT_FLOAT_EQ(vals[0], 1.f);
    EXPECT_FLOAT_EQ(vals[1], 2.f);
    EXPECT_FLOAT_EQ(vals[2], 3.f);
    EXPECT_FLOAT_EQ(vals[3], 4.f);
}