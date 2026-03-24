#pragma once
#include "Scene.h"
//
//namespace Jam
//{
//
//struct RenderView;
//class Transform;
//class Material;
//class Mesh;
//class Timer;
//struct Frustum;
//class RenderGraph;
//
//struct VertexPosTex2D
//{
//    Vec2 position = Numbers<Vec2>::k_zero;
//    Vec2 texCoord = Numbers<Vec2>::k_zero;
//
//    inline static VertexLayout s_layout = []
//    {
//        VertexLayout vl = {};
//        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float2);
//        vl.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
//        return vl;
//    }();
//};
//
//struct VertexStaticMesh
//{
//    Vec3 position  = Numbers<Vec3>::k_zero;
//    Vec3 normal    = Numbers<Vec3>::k_zero;
//    Vec3 tangent   = Numbers<Vec3>::k_zero;
//    Vec2 texCoord0 = Numbers<Vec2>::k_zero;
//    Vec2 texCoord1 = Numbers<Vec2>::k_zero;
//
//    inline static VertexLayout s_layout = []
//    {
//        VertexLayout vl = {};
//        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
//        vl.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);
//        vl.Push(eVertexAttribute::Tangent, eAttributeFormat::Float3);
//        vl.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
//        vl.Push(eVertexAttribute::TexCoord1, eAttributeFormat::Float2);
//        return vl;
//    }();
//};
//
//struct VertexPos3D
//{
//    Vec3 position = Numbers<Vec3>::k_zero;
//
//    inline static VertexLayout s_layout = []
//    {
//        VertexLayout vl;
//        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
//        return vl;
//    }();
//};
//
//class BasicRenderer
//{
//public:
//    static void InitFileWatcher();
//    static void Shutdown();
//    static void Render(const RenderView& _renderView, const Ref<Scene>& _scene);
//
//    [[nodiscard]] static TextureHandle GetDepthTexture();
//};
//
//}   // namespace Jam
