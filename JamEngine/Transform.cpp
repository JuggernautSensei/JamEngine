#include "pch.h"

#include "Transform.h"

#include "MathUtilities.h"

namespace jam
{

Mat4 Transform::CreateWorld() const
{
    return jam::CreateWorld(translate, rotate, scale);
}

Mat4 Transform::CreateView() const
{
    return CreateViewLookTo(translate, Forward());
}

Transform Transform::ToParent(const Transform& _parentTransform) const
{
    Transform result;
    result.scale     = scale * _parentTransform.scale;
    result.rotate    = _parentTransform.rotate * rotate;
    result.translate = Vec3::Transform(translate * _parentTransform.scale, _parentTransform.rotate) + _parentTransform.translate;
    return result;
}

Transform Transform::FromParent(const Transform& _parentTransform) const
{
    Transform invParent = _parentTransform.Inverse();
    return ToParent(invParent);
}

Transform Transform::Inverse() const
{
    Transform inv;
    inv.scale = Vec3(1.f / scale.x, 1.f / scale.y, 1.f / scale.z);
    rotate.Inverse(inv.rotate);
    inv.translate = Vec3::Transform(-translate, inv.rotate) * inv.scale;
    return inv;
}

}   // namespace jam