#include "pch.h"
#include "FlyingCamera.h"

void FlyingCamera::Update(
    const float _dt)
{
    Input& input = GetInput();
    if (input.IsKeyDown(eKey::A))
    {
        const Vec3 left = trans.GetLeft();
        trans.AddTranslation(left * moveSpeed * _dt);
    }
    if (input.IsKeyDown(eKey::D))
    {
        const Vec3 right = trans.GetRight();
        trans.AddTranslation(right * moveSpeed * _dt);
    }
    if (input.IsKeyDown(eKey::W))
    {
        const Vec3 forward = trans.GetForward();
        trans.AddTranslation(forward * moveSpeed * _dt);
    }
    if (input.IsKeyDown(eKey::S))
    {
        const Vec3 back = trans.GetBackward();
        trans.AddTranslation(back * moveSpeed * _dt);
    }
    if (input.IsKeyDown(eKey::Q))
    {
        const Vec3 down = trans.GetDown();
        trans.AddTranslation(down * moveSpeed * _dt);
    }
    if (input.IsKeyDown(eKey::E))
    {
        const Vec3 up = trans.GetUp();
        trans.AddTranslation(up * moveSpeed * _dt);
    }

    dragger.Update();
    const auto [dx, dy] = dragger.GetDragDelta();
    if (dx != 0 || dy != 0)
    {
        pitchRad += dy * rotSpeed;
        yawRad += dx * rotSpeed;
        trans.SetRotation(pitchRad, yawRad, 0.f);
    }
}