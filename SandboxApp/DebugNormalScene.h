#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/RenderGraph.h"

class DebugNormalScene : public Scene
{
    JAM_SCENE_BODY(DebugNormalScene);

    struct CBNoWIT
    {
        static constexpr Int32 kSlot = 0;

        Mtx4 world;
    };

    struct CBWithWIT
    {
        static constexpr Int32 kSlot = 0;

        Mtx4 world;
        Mtx4 invTransWorld;
    };

    struct CBCamera
    {
        static constexpr Int32 kSlot = 1;

        Mtx4  viewProj;
        Vec3  eyePos;
        float padding;
    };

public:
    DebugNormalScene();

    void OnUpdate(float _dt) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void InitMeshes_();
    void InitRenderGraph_();
    void OnResize(Int32 _width, Int32 _height) override;

    void AddProgram_VP_(StringView _name, StringView _path);
    void AddProgram_VPG_(StringView _name, StringView _path);
    void RenderMeshes_() const;
    void RenderFullScreenTri_() const;
    void UpdateEntityConstants_(const Transform& _trans) const;

    Ref<Mesh> m_pSphere     = nullptr;
    Transform m_sphereTrans = {};

    Ref<Mesh> m_pBunny     = nullptr;
    Transform m_bunnyTrans = {};

    Ref<Mesh> m_pFsTri = nullptr;

    ConstantBufferHandle m_noWitCbh   = kInvalidHandle;
    ConstantBufferHandle m_withWitCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh     = kInvalidHandle;

    RenderGraph  m_graph  = {};
    FlyingCamera m_camera = {};

    float m_dAnim       = 1.f;
    bool  m_bAnimate    = true;
    bool  m_bUseWit     = false;
    bool  m_bDrawNormal = false;
};
