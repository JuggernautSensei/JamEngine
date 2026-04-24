#pragma once
#include "FlyingCamera.h"

class TextureMappingScene : public Scene
{
    JAM_SCENE_BODY(TextureMappingScene);

    enum class eTextureMappingFlags : UInt32
    {
        None   = 0,
        Albedo = 1 << 0,
        Ao     = 1 << 1,
        Normal = 1 << 2
    };

public:
    TextureMappingScene();
    ~TextureMappingScene() noexcept override;

    TextureMappingScene(const TextureMappingScene&)                = delete;
    TextureMappingScene& operator=(const TextureMappingScene&)     = delete;
    TextureMappingScene(TextureMappingScene&&) noexcept            = delete;
    TextureMappingScene& operator=(TextureMappingScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    ProgramHandle m_ph = kInvalidHandle;

    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh    = kInvalidHandle;
    ConstantBufferHandle m_lightCbh  = kInvalidHandle;

    FrameBufferHandle m_fbh    = {};
    TextureHandle     m_fbTexh = {};   // not owned

    bool  m_bAnimate = true;
    float m_dRot     = ToRad(45.f);
    float m_rot      = 0.f;

    Ref<Mesh>    m_pMesh   = nullptr;
    Ref<Texture> m_pAlbedo = nullptr;
    Ref<Texture> m_pAo     = nullptr;
    Ref<Texture> m_pNormal = nullptr;

    FlyingCamera m_camera = {};

    Flags<eTextureMappingFlags> m_textureMappingFlags = eTextureMappingFlags::Albedo | eTextureMappingFlags::Ao | eTextureMappingFlags::Normal;

    Vec3 m_lightPos     = Vec3 { 0.f, 5.f, -5.f };
    Vec3 m_lightLookAt  = Vec3 { 0.f, 0.f, 0.f };
    Vec3 m_lightColor   = Vec3 { 1.f, 1.f, 1.f };
    Vec3 m_ambientColor = Vec3 { 0.3f, 0.3f, 0.3f };
};
