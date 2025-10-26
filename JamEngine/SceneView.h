#pragma once
#include "GraphicsBase.h"

namespace jam
{

struct SceneView
{
    FrameBufferHandle frameBufferHandle = k_invalidHandle;
    Viewport          viewport          = {};

    Vec3 eye  = Vec3::Zero;
    Mat4 view = Mat4::Identity;
    Mat4 proj = Mat4::Identity;
};

}   // namespace jam