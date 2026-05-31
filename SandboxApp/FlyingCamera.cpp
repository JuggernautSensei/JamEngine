#include "pch.h"
#include "FlyingCamera.h"

void FlyingCamera::Update(
    const float _dt)
{
    Input& input = GetInput();
    if (input.IsKeyDown(eKey::A))
    {
        const Vec3 left = transform.GetLeft();
        transform.AddTranslation(left * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::D))
    {
        const Vec3 right = transform.GetRight();
        transform.AddTranslation(right * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::W))
    {
        const Vec3 forward = transform.GetForward();
        transform.AddTranslation(forward * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::S))
    {
        const Vec3 back = transform.GetBackward();
        transform.AddTranslation(back * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::Q))
    {
        const Vec3 down = transform.GetDown();
        transform.AddTranslation(down * dMove * _dt);
    }

    if (input.IsKeyDown(eKey::E))
    {
        const Vec3 up = transform.GetUp();
        transform.AddTranslation(up * dMove * _dt);
    }

    dragger.Update();
    const Vec2i dd = dragger.GetDragDelta();
    if (dd.x != 0 || dd.y != 0)
    {
        euler.pitch += static_cast<float>(dd.y) * dRot;
        euler.pitch = Clamp(euler.pitch, ToRad(-89.f), ToRad(89.f));
        euler.yaw += static_cast<float>(dd.x) * dRot;
        transform.SetRotation(euler);
    }
}

Vec3 FlyingCamera::GetEyePos() const
{
    return transform.GetTranslation();
}

const Mtx4& FlyingCamera::GetProj() const
{
    return camera.GetProj();
}

Mtx4 FlyingCamera::CalcView() const
{
    return transform.GetInvSRT();
}

Mtx4 FlyingCamera::CalcViewProj() const
{
    return CalcView() * GetProj();
}

void FlyingCamera::FitAspectRatioAsBackBufferResolution()
{
    Graphics&         gfx = GetGraphics();
    const BackBuffer& bb  = gfx.GetBackBuffer();
    camera.SetAspectRatio(static_cast<float>(bb.width) / static_cast<float>(bb.height));
}
