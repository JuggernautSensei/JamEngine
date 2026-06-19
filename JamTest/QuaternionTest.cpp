#include "pch.h"
#include <gtest/gtest.h>

using namespace Jam;

// ─────────────────────────────────────────────
// 헬퍼
// ─────────────────────────────────────────────

static constexpr float kTol = 1e-4f;

static void ExpectQuatEq(const Quat& _a, const Quat& _b, float _tol = kTol)
{
    // 쿼터니언은 q와 -q가 동일한 회전을 나타내므로 두 경우 모두 허용
    const bool bSame = std::abs(_a.x - _b.x) < _tol && std::abs(_a.y - _b.y) < _tol && std::abs(_a.z - _b.z) < _tol && std::abs(_a.w - _b.w) < _tol;

    const bool bNeg = std::abs(_a.x + _b.x) < _tol && std::abs(_a.y + _b.y) < _tol && std::abs(_a.z + _b.z) < _tol && std::abs(_a.w + _b.w) < _tol;

    EXPECT_TRUE(bSame || bNeg)
        << "  a=(" << _a.x << ", " << _a.y << ", " << _a.z << ", " << _a.w << ")\n"
        << "  b=(" << _b.x << ", " << _b.y << ", " << _b.z << ", " << _b.w << ")";
}

static void ExpectVec3Eq(const Vec3& _a, const Vec3& _b, float _tol = kTol)
{
    EXPECT_NEAR(_a.x, _b.x, _tol);
    EXPECT_NEAR(_a.y, _b.y, _tol);
    EXPECT_NEAR(_a.z, _b.z, _tol);
}

// ─────────────────────────────────────────────
// kIdentity
// ─────────────────────────────────────────────

TEST(Quat, Identity)
{
    const Quat q = Quat::kIdentity;
    EXPECT_FLOAT_EQ(q.x, 0.f);
    EXPECT_FLOAT_EQ(q.y, 0.f);
    EXPECT_FLOAT_EQ(q.z, 0.f);
    EXPECT_FLOAT_EQ(q.w, 1.f);
}

// ─────────────────────────────────────────────
// IsNormalized
// ─────────────────────────────────────────────

TEST(Quat, IsNormalized_Identity)
{
    EXPECT_TRUE(Quat::kIdentity.IsNormalized());
}

TEST(Quat, IsNormalized_NotNormalized)
{
    const Quat q = { 1.f, 1.f, 1.f, 1.f };
    EXPECT_FALSE(q.IsNormalized());
}

TEST(Quat, IsNormalized_AfterNormalize)
{
    Quat q = { 1.f, 2.f, 3.f, 4.f };
    q.Normalize();
    EXPECT_TRUE(q.IsNormalized());
}

// ─────────────────────────────────────────────
// FromAxisAngle / ToAxisAngle 왕복
// ─────────────────────────────────────────────

TEST(Quat, FromAxisAngle_XAxis90)
{
    const Quat q = Quat::FromAxisAngle({ 1.f, 0.f, 0.f }, kPi * 0.5f);
    EXPECT_TRUE(q.IsNormalized());
    EXPECT_NEAR(q.x, std::sin(kPi * 0.25f), kTol);
    EXPECT_NEAR(q.w, std::cos(kPi * 0.25f), kTol);
}

TEST(Quat, AxisAngleRoundTrip)
{
    const Vec3  axis   = Vec3 { 0.f, 1.f, 0.f };
    const float angle  = kPi / 3.f;
    const Quat  q      = Quat::FromAxisAngle(axis, angle);
    const auto  result = q.ToAxisAngle();

    EXPECT_NEAR(result.rad, angle, kTol);
    ExpectVec3Eq(result.axis, axis);
}

// ─────────────────────────────────────────────
// FromEuler / ToEuler 왕복 (Ry·Rx·Rz)
// ─────────────────────────────────────────────

TEST(Quat, EulerRoundTrip_XOnly)
{
    const Vec3 euler = { kPi / 4.f, 0.f, 0.f };
    const Vec3 back  = Quat::FromEuler(euler).ToEuler(TODO);
    EXPECT_NEAR(back.x, euler.x, kTol);
    EXPECT_NEAR(back.y, 0.f, kTol);
    EXPECT_NEAR(back.z, 0.f, kTol);
}

TEST(Quat, EulerRoundTrip_YOnly)
{
    const Vec3 euler = { 0.f, kPi / 3.f, 0.f };
    const Vec3 back  = Quat::FromEuler(euler).ToEuler(TODO);
    EXPECT_NEAR(back.x, 0.f, kTol);
    EXPECT_NEAR(back.y, euler.y, kTol);
    EXPECT_NEAR(back.z, 0.f, kTol);
}

TEST(Quat, EulerRoundTrip_ZOnly)
{
    const Vec3 euler = { 0.f, 0.f, kPi / 5.f };
    const Vec3 back  = Quat::FromEuler(euler).ToEuler(TODO);
    EXPECT_NEAR(back.x, 0.f, kTol);
    EXPECT_NEAR(back.y, 0.f, kTol);
    EXPECT_NEAR(back.z, euler.z, kTol);
}

// ─────────────────────────────────────────────
// FromToRotation
// ─────────────────────────────────────────────

TEST(Quat, FromToRotation_SameDirection)
{
    const Vec3 v = { 0.f, 1.f, 0.f };
    const Quat q = Quat::FromToRotation(v, v);
    ExpectQuatEq(q, Quat::kIdentity);
}

TEST(Quat, FromToRotation_OppositeDirection)
{
    const Vec3 from = { 0.f, 1.f, 0.f };
    const Vec3 to   = { 0.f, -1.f, 0.f };
    const Quat q    = Quat::FromToRotation(from, to);
    EXPECT_TRUE(q.IsNormalized());

    // from을 q로 회전하면 to와 같아야 한다
    const Vec3 rotated = q.Rotate(from);
    ExpectVec3Eq(rotated, to);
}

TEST(Quat, FromToRotation_Orthogonal)
{
    const Vec3 from    = { 1.f, 0.f, 0.f };
    const Vec3 to      = { 0.f, 1.f, 0.f };
    const Quat q       = Quat::FromToRotation(from, to);
    const Vec3 rotated = q.Rotate(from);
    ExpectVec3Eq(rotated, to);
}

TEST(Quat, FromToRotation_ResultIsNormalized)
{
    const Quat q = Quat::FromToRotation({ 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f });
    EXPECT_TRUE(q.IsNormalized());
}

// ─────────────────────────────────────────────
// Rotate
// ─────────────────────────────────────────────

TEST(Quat, Rotate_Identity)
{
    const Vec3 v       = { 1.f, 2.f, 3.f };
    const Vec3 rotated = Quat::kIdentity.Rotate(v);
    ExpectVec3Eq(rotated, v);
}

TEST(Quat, Rotate_X90)
{
    // X축 90도 회전: +Y → +Z
    const Quat q       = Quat::FromAxisAngle({ 1.f, 0.f, 0.f }, kPi * 0.5f);
    const Vec3 rotated = q.Rotate({ 0.f, 1.f, 0.f });
    ExpectVec3Eq(rotated, { 0.f, 0.f, 1.f });
}

TEST(Quat, Rotate_Y90)
{
    // Y축 90도 회전: +Z → +X
    const Quat q       = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    const Vec3 rotated = q.Rotate({ 0.f, 0.f, 1.f });
    ExpectVec3Eq(rotated, { 1.f, 0.f, 0.f });
}

// ─────────────────────────────────────────────
// Conjugate / Inverse
// ─────────────────────────────────────────────

TEST(Quat, Conjugate)
{
    const Quat q    = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi / 4.f);
    const Quat conj = q.Conjugate();
    EXPECT_FLOAT_EQ(conj.x, -q.x);
    EXPECT_FLOAT_EQ(conj.y, -q.y);
    EXPECT_FLOAT_EQ(conj.z, -q.z);
    EXPECT_FLOAT_EQ(conj.w, q.w);
}

TEST(Quat, InverseTimesOriginal_IsIdentity)
{
    const Quat q      = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi / 4.f);
    const Quat result = q.Inverse() * q;
    ExpectQuatEq(result, Quat::kIdentity);
}

// ─────────────────────────────────────────────
// Dot
// ─────────────────────────────────────────────

TEST(Quat, Dot_Identity)
{
    EXPECT_NEAR(Quat::kIdentity.Dot(Quat::kIdentity), 1.f, kTol);
}

TEST(Quat, Dot_Orthogonal)
{
    const Quat a = Quat::FromAxisAngle({ 1.f, 0.f, 0.f }, kPi * 0.5f);
    const Quat b = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    // 두 쿼터니언이 직교하지는 않지만 dot이 0.5임을 확인
    EXPECT_NEAR(a.Dot(b), 0.5f, kTol);
}

// ─────────────────────────────────────────────
// Lerp (NLerp) — 정규화 확인
// ─────────────────────────────────────────────

TEST(Quat, Lerp_T0_IsA)
{
    const Quat a = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, 0.f);
    const Quat b = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    ExpectQuatEq(a.Lerp(b, 0.f), a);
}

TEST(Quat, Lerp_T1_IsB)
{
    const Quat a = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, 0.f);
    const Quat b = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    ExpectQuatEq(a.Lerp(b, 1.f), b);
}

TEST(Quat, Lerp_ResultIsNormalized)
{
    const Quat a      = Quat::FromAxisAngle({ 1.f, 0.f, 0.f }, 0.f);
    const Quat b      = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    const Quat result = a.Lerp(b, 0.5f);
    EXPECT_TRUE(result.IsNormalized());
}

// ─────────────────────────────────────────────
// Slerp
// ─────────────────────────────────────────────

TEST(Quat, Slerp_T0_IsA)
{
    const Quat a = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, 0.f);
    const Quat b = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    ExpectQuatEq(a.Slerp(b, 0.f), a);
}

TEST(Quat, Slerp_T1_IsB)
{
    const Quat a = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, 0.f);
    const Quat b = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    ExpectQuatEq(a.Slerp(b, 1.f), b);
}

TEST(Quat, Slerp_T05_HalfAngle)
{
    const Quat a      = Quat::kIdentity;
    const Quat b      = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    const Quat result = a.Slerp(b, 0.5f);
    const Quat expect = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.25f);
    ExpectQuatEq(result, expect);
}

// ─────────────────────────────────────────────
// Angle
// ─────────────────────────────────────────────

TEST(Quat, Angle_SameQuat)
{
    const Quat q = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi / 3.f);
    EXPECT_NEAR(q.Angle(q), 0.f, kTol);
}

TEST(Quat, Angle_90Degrees)
{
    const Quat a = Quat::kIdentity;
    const Quat b = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    EXPECT_NEAR(a.Angle(b), kPi * 0.5f, kTol);
}

TEST(Quat, Angle_180Degrees)
{
    const Quat a = Quat::kIdentity;
    const Quat b = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi);
    EXPECT_NEAR(a.Angle(b), kPi, kTol);
}

// ─────────────────────────────────────────────
// Exp / Log 왕복
// ─────────────────────────────────────────────

TEST(Quat, LogExp_RoundTrip)
{
    const Quat q      = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi / 3.f);
    const Quat result = q.Log().Exp();
    ExpectQuatEq(result, q);
}

TEST(Quat, Log_Identity_IsZeroVector)
{
    // log(identity) = (0,0,0,0)
    const Quat logQ = Quat::kIdentity.Log();
    EXPECT_NEAR(logQ.x, 0.f, kTol);
    EXPECT_NEAR(logQ.y, 0.f, kTol);
    EXPECT_NEAR(logQ.z, 0.f, kTol);
    EXPECT_NEAR(logQ.w, 0.f, kTol);
}

// ─────────────────────────────────────────────
// Pow
// ─────────────────────────────────────────────

TEST(Quat, Pow_T0_IsIdentity)
{
    const Quat q = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi / 3.f);
    ExpectQuatEq(q.Pow(0.f), Quat::kIdentity);
}

TEST(Quat, Pow_T1_IsSelf)
{
    const Quat q = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi / 3.f);
    ExpectQuatEq(q.Pow(1.f), q);
}

TEST(Quat, Pow_T05_HalfAngle)
{
    const float angle  = kPi / 3.f;
    const Quat  q      = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, angle);
    const Quat  result = q.Pow(0.5f);
    const Quat  expect = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, angle * 0.5f);
    ExpectQuatEq(result, expect);
}

// ─────────────────────────────────────────────
// Squad — 양 끝점 도달 확인
// ─────────────────────────────────────────────

TEST(Quat, Squad_T0_IsQ1)
{
    const Quat q0 = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, 0.f);
    const Quat q1 = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.25f);
    const Quat q2 = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    const Quat q3 = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.75f);

    const Quat s1     = Quat::SquadSetup(q0, q1, q2);
    const Quat s2     = Quat::SquadSetup(q1, q2, q3);
    const Quat result = Quat::Squad(q1, q2, s1, s2, 0.f);
    ExpectQuatEq(result, q1);
}

TEST(Quat, Squad_T1_IsQ2)
{
    const Quat q0 = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, 0.f);
    const Quat q1 = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.25f);
    const Quat q2 = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.5f);
    const Quat q3 = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.75f);

    const Quat s1     = Quat::SquadSetup(q0, q1, q2);
    const Quat s2     = Quat::SquadSetup(q1, q2, q3);
    const Quat result = Quat::Squad(q1, q2, s1, s2, 1.f);
    ExpectQuatEq(result, q2);
}

TEST(Quat, Squad_ResultIsNormalized)
{
    const Quat q0 = Quat::FromAxisAngle({ 1.f, 0.f, 0.f }, 0.f);
    const Quat q1 = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi * 0.25f);
    const Quat q2 = Quat::FromAxisAngle({ 0.f, 0.f, 1.f }, kPi * 0.5f);
    const Quat q3 = Quat::FromAxisAngle({ 1.f, 1.f, 0.f }, kPi * 0.75f);

    const Quat s1     = Quat::SquadSetup(q0, q1, q2);
    const Quat s2     = Quat::SquadSetup(q1, q2, q3);
    const Quat result = Quat::Squad(q1, q2, s1, s2, 0.5f);
    EXPECT_TRUE(result.IsNormalized());
}

// ─────────────────────────────────────────────
// 연산자
// ─────────────────────────────────────────────

TEST(Quat, Multiply_IdentityPreservesQ)
{
    const Quat q = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi / 4.f);
    ExpectQuatEq(Quat::kIdentity * q, q);
    ExpectQuatEq(q * Quat::kIdentity, q);
}

TEST(Quat, Multiply_ComposeRotations)
{
    // X 90도 두 번 = X 180도
    const Quat q90  = Quat::FromAxisAngle({ 1.f, 0.f, 0.f }, kPi * 0.5f);
    const Quat q180 = Quat::FromAxisAngle({ 1.f, 0.f, 0.f }, kPi);
    ExpectQuatEq(q90 * q90, q180);
}

TEST(Quat, Negate_SameRotation)
{
    const Quat q = Quat::FromAxisAngle({ 0.f, 1.f, 0.f }, kPi / 3.f);
    const Vec3 v = { 1.f, 2.f, 3.f };
    ExpectVec3Eq(q.Rotate(v), (-q).Rotate(v));
}

TEST(Quat, ScalarMultiply)
{
    const Quat q      = Quat::kIdentity;
    const Quat result = q * 2.f;
    EXPECT_NEAR(result.w, 2.f, kTol);
}

TEST(Quat, IndexOperator)
{
    const Quat q = { 1.f, 2.f, 3.f, 4.f };
    EXPECT_FLOAT_EQ(q[0], 1.f);
    EXPECT_FLOAT_EQ(q[1], 2.f);
    EXPECT_FLOAT_EQ(q[2], 3.f);
    EXPECT_FLOAT_EQ(q[3], 4.f);
}