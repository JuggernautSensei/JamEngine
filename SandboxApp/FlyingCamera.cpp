#include "pch.h"
#include "FlyingCamera.h"

void FlyingCamera::Update(
    const float _dt)
{
    Input& input = GetInput();
    if (input.IsKeyDown(eKey::A))
    {
        const Vec3 left = trans.GetLeft();
        trans.AddTranslation(left * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::D))
    {
        const Vec3 right = trans.GetRight();
        trans.AddTranslation(right * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::W))
    {
        const Vec3 forward = trans.GetForward();
        trans.AddTranslation(forward * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::S))
    {
        const Vec3 back = trans.GetBackward();
        trans.AddTranslation(back * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::Q))
    {
        const Vec3 down = trans.GetDown();
        trans.AddTranslation(down * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::E))
    {
        const Vec3 up = trans.GetUp();
        trans.AddTranslation(up * dMove * _dt);
    }

    dragger.Update();
    const Vec2i dd = dragger.GetDragDelta();
    if (dd.x != 0 || dd.y != 0)
    {
        euler.pitch += dd.y * dRot;
        euler.pitch = Clamp(euler.pitch, ToRad(-89.f), ToRad(89.f));
        euler.yaw += dd.x * dRot;
        trans.SetRotation(euler);
    }
}

Vec3 FlyingCamera::GetEyePos() const
{
    return trans.GetTranslation();
}

const Mtx4& FlyingCamera::GetProj() const
{
    return cmr.GetProj();
}

Mtx4 FlyingCamera::CalcViewProj() const
{
    return trans.CalcInvSRT() * cmr.GetProj();
}

Mtx4 FlyingCamera::CalcView() const
{
    return trans.CalcInvSRT();
}
