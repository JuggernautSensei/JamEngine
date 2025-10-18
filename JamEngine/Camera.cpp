#include "pch.h"

#include "Camera.h"

#include "Transform.h"

namespace jam
{

Frustum Camera::CreateFrustum() const
{
    return jam::CreateFrustum(CreateProj());
}

Frustum Camera::CreateFrustumInWorld(const Transform& _transform) const
{
    Frustum frustumInV = jam::CreateFrustum(CreateProj());
    Frustum frustumInW;
    frustumInW.Transform(frustumInV, _transform.CreateWorld());
    return frustumInW;
}

Mat4 Camera::CreateProj() const
{
    if (projection == eProjection::Perspective)
    {
        const float aspectRatio = width / height;
        return CreatePerspectiveFov(fovYRad, aspectRatio, nearZ, farZ);
    }
    else
    {
        return CreateOrthographic(width, height, nearZ, farZ);
    }
}

}   // namespace jam