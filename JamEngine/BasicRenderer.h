#pragma once
#include "BasicRendererBase.h"
#include "RenderPass.h"

namespace jam
{

struct Transform;
class Material;
class Mesh;
class Timer;
struct Frustum;
class RenderGraph;
struct Environment;
class CullingSystem;

class BasicRenderer
{
public:
    static void Init();
    static void Shutdown();

    static void Render(const Vec3& _eyePos, const Mat4& _view, const Mat4& _proj, const Environment& _env);

    JAM_NODISCARD static const Timer&   GetRendererCPUProfileTimer();
    JAM_NODISCARD static CullingSystem& GetCullingSystem();

    // draw call helpers
    static void Submit(const Mesh& _mesh, const Material& _material, const Transform& _transform);
    static void Submit(const Mesh& _mesh, const Transform& _transform);

    // render graph
    static void CompileGraph();
    static void AddPass(const RenderPass& _pass);

    static RenderPass CreatePBRGBufferPass(const PBRGBufferPassDesc& _desc);
    static RenderPass CreatePBRLightingPass(const PBRLightPassDesc& _desc);
};

}   // namespace jam
