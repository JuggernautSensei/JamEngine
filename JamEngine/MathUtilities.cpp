#include "pch.h"

#include "MathUtilities.h"

namespace jam
{

Mat4 CreateViewMatrixFromDirection(const Vec3& position, const Vec3& lookDir)
{
    using namespace DirectX;

    assert(!XMVector3Equal(lookDir, XMVectorZero()));
    assert(!XMVector3IsInfinite(lookDir));

    const XMVECTOR viewDir = XMVector3Normalize(lookDir);
    const XMVECTOR worldUp = Vec3::Up;

    XMVECTOR rightDir = XMVector3Cross(worldUp, viewDir);
    if (XMVector3Equal(rightDir, XMVectorZero()))
    {
        rightDir = XMVector3Normalize(XMVector3Cross(Vec3::Right, viewDir));
    }

    rightDir = XMVector3Normalize(rightDir);

    const XMVECTOR upDir = XMVector3Cross(viewDir, rightDir);
    const XMVECTOR negPosition = XMVectorNegate(position);

    const XMVECTOR dotX = XMVector3Dot(rightDir, negPosition);
    const XMVECTOR dotY = XMVector3Dot(upDir, negPosition);
    const XMVECTOR dotZ = XMVector3Dot(viewDir, negPosition);

    XMMATRIX viewMatrix;
    viewMatrix.r[0] = XMVectorSelect(dotX, rightDir, g_XMSelect1110.v);  
    viewMatrix.r[1] = XMVectorSelect(dotY, upDir, g_XMSelect1110.v);     
    viewMatrix.r[2] = XMVectorSelect(dotZ, viewDir, g_XMSelect1110.v);   
    viewMatrix.r[3] = g_XMIdentityR3.v;                                  

    viewMatrix = XMMatrixTranspose(viewMatrix);
    return viewMatrix;
}

}