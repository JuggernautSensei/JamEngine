#pragma once
#include "Camera.h"
#include "Components.h"
#include "Environment.h"

namespace jam
{

struct Environment;
struct Transform;
struct DrawItem;
struct Camera;
struct Light;
class Mesh;
class Material;
class ImageTexture;

}   // namespace jam

namespace jam::r3d
{

void Init();
void Shutdown();

void SetCamera(const Camera& _camera, const Transform& _cameraTrans); // 내부적으로 _viewFrustumInW을 계산
void SetCamera(const Camera& _camera, const Transform& _cameraTrans, const Frustum& _viewFrustumInW);

namespace pass::pbr_gbuffer
{
    void SetupPass(bool _bSetDefaultState = true);
    void Submit(const DrawItem& _item);
}   // namespace pass::pbr_gbuffer

namespace pass::light_volume
{
    void SetupPass(bool _bSetDefaultState = true);
    void Submit(const Light& _light, const Transform& _transform);
    void Submit(const Environment& _env);
}   // namespace pass::light_volume

namespace pass::pbr_lighting
{
    void SetupPass(bool _bSetDefaultState = true);
    void SetEnvironment(const Environment& _env);
    void Submit();
}   // namespace pass::pbr_lighting

namespace pass::skybox
{
    void SetupPass(bool _bSetDefaultState = true);
    void SetEnvironment(const Environment& _env);
    void Submit();
}   // namespace pass::skybox

}   // namespace jam::r3d