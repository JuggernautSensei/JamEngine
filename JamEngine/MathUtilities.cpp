#include "pch.h"

#include "MathUtilities.h"
#include <Random>
using namespace jam;

namespace jam::details
{
std::mt19937 g_randomGenerator = std::mt19937 {};
UInt32       g_randomSeed      = 0;
}   // namespace jam::details

namespace jam
{

Mat4 CreateTransformLookTo(const Vec3& _forward)
{
    using namespace DirectX;

    assert(!XMVector3Equal(_forward, XMVectorZero()));
    assert(!XMVector3IsInfinite(_forward));

    const XMVECTOR forward = XMVector3Normalize(_forward);

    // 오른쪽 벡터 계산
    XMVECTOR up    = Vec3::Up;
    XMVECTOR right = XMVector3Cross(up, forward);
    if (XMVector3Equal(right, XMVectorZero()))   // forward가 Up과 평행한 경우
    {
        // Right 벡터를 사용하여 교차 곱 계산
        right = XMVector3Cross(Vec3::Right, forward);
    }

    right = XMVector3Normalize(right);
    up    = XMVector3Normalize(XMVector3Cross(forward, right));

    XMMATRIX mat;
    mat.r[0] = right;
    mat.r[1] = up;
    mat.r[2] = forward;
    mat.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    return mat;
}

Mat4 CreateWorld(const Vec3& _translate, const Vec3& _forward, const Vec3& _scale)
{
    using namespace DirectX;

    XMMATRIX lookToMat = CreateTransformLookTo(_forward);
    lookToMat.r[0]     = XMVectorMultiply(lookToMat.r[0], _scale);
    lookToMat.r[1]     = XMVectorMultiply(lookToMat.r[1], _scale);
    lookToMat.r[2]     = XMVectorMultiply(lookToMat.r[2], _scale);
    lookToMat.r[3]     = XMVectorSetW(_translate, 1.0f);
    return lookToMat;
}

Mat4 CreateWorld(const Vec3& _translate, const Quat& _quat, const Vec3& _scale)
{
    Vec3 forward = Vec3::Transform(Vec3::Forward, _quat);
    return CreateWorld(_translate, forward, _scale);
}

Mat4 CreateViewLookTo(const Vec3& _position, const Vec3& _forward)
{
    using namespace DirectX;

    XMMATRIX lookToMat = CreateTransformLookTo(_forward);

    // 평행이동 역행렬 (= 역원)
    const XMVECTOR nPos = XMVectorNegate(_position);
    const XMVECTOR dotX = XMVector3Dot(lookToMat.r[0], nPos);
    const XMVECTOR dotY = XMVector3Dot(lookToMat.r[1], nPos);
    const XMVECTOR dotZ = XMVector3Dot(lookToMat.r[2], nPos);

    const XMVECTOR right = lookToMat.r[0];
    const XMVECTOR up    = lookToMat.r[1];
    const XMVECTOR fwd   = lookToMat.r[2];

    lookToMat.r[0] = XMVectorSelect(dotX, right, g_XMSelect1110.v);
    lookToMat.r[1] = XMVectorSelect(dotY, up, g_XMSelect1110.v);
    lookToMat.r[2] = XMVectorSelect(dotZ, fwd, g_XMSelect1110.v);
    lookToMat.r[3] = g_XMIdentityR3.v;

    // 회전 역행렬 (= 전치 행렬)
    return XMMatrixTranspose(lookToMat);
}

Mat4 CreateViewLookAt(const Vec3& _position, const Vec3& _focusPoint)
{
    const Vec3 forward = (_focusPoint - _position);
    return CreateViewLookTo(_position, forward);
}

Mat4 CreateOrthographic(const float width, const float height, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

Mat4 CreateOrthographic(const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);
}

Mat4 CreatePerspectiveFov(const float fovYRad, const float aspectRatio, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixPerspectiveFovLH(fovYRad, aspectRatio, nearZ, farZ);
}

AxisAngle QuatToAxisAngle(const Quat& _q)
{
    Quat q = _q;
    q.Normalize();

    Vec3 axis = { q.x, q.y, q.z };
    axis.Normalize();

    float angle = 2.f * std::acosf(q.w);
    return { axis, angle };
}

}   // namespace jam