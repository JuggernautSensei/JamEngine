#include "pch.h"

#include "Camera.h"

#include "Transform.h"

namespace jam
{

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