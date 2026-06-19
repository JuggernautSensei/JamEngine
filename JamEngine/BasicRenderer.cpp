#include "pch.h"

#include "BasicRenderer.h"
//
//#include "EditorWidgets.h"
//#include "EmbeddedShaderGlue.h"
//#include "Environment.h"
//#include "EnvironmentComponent.h"
//#include "Graphics.h"
//#include "Material.h"
//#include "Memory.h"
//#include "MeshFactory.h"
//#include "Profile.h"
//#include "RenderGraph.h"
//#include "RenderableComponent.h"
//#include "SceneComponent.h"
//#include "Shaders/PrecompiledShaders/PrecompiledJamShader.h"
//#include "Timer.h"
//
//namespace Jam
//{
//
//enum class eAttachment
//{
//    RenderTarget,
//    DepthStencil
//};
//
//struct BasicRendererContext
//{
//    // cbuffer unload
//    void UploadTransform(const Matrix& _world) const;
//    void UploadCamera(Vec3 _eye, const Matrix& _view, const Matrix& _proj) const;
//    void UploadEnvironment(const Environment& _env) const;
//    void UploadScreen(Int32 _width, Int32 _height) const;
//    void UploadMaterial(const Ref<Material>& _material) const;
//    void UploadPointLight(Vec3 _radiance, float _lightIntensity, Vec3 _pos, float _fallOffStart, float _fallOffEnd, bool _bShadow) const;
//    void UploadSpotLight(Vec3 _radiance, float _lightIntensity, Vec3 _pos, Vec3 _dir, float _fallOffStart, float _fallOffEnd, float _innerConeAngle, float _outerConeAngle, bool _bShadow) const;
//
//    // factory
//    [[nodiscard]] TextureHandle     CreateTexture2D(Int32 _width, Int32 _height, eTextureFormat _format, eAttachment _type, StringView _name) const;
//    [[nodiscard]] TextureHandle     CreateTextureCube(Int32 _width, Int32 _height, eTextureFormat _format, eAttachment _type, StringView _name) const;
//    [[nodiscard]] TextureHandle     CreateTextureArray(Int32 _width, Int32 _height, Int32 _numLayers, eTextureFormat _format, eAttachment _type, StringView _name) const;
//    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(Span<const TextureHandle> _attachment, bool _bDestroyTexture, StringView _name) const;
//    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(TextureHandle _texh, bool _bDestroyTexture, StringView _name) const;
//    [[nodiscard]] FrameBufferHandle CreateFrameBuffer(Int32 _width, Int32 _height, eTextureFormat _format, StringView _name) const;
//
//    // submit
//    void Submit(const Ref<Mesh>& _mesh, const Ref<Material>& _material, const Matrix& _world) const;
//    void SubmitFullScreenTri() const;
//    void SubmitSkyboxCube() const;
//
//    // misc
//    void BakeSSAOKernel() const;
//    void BakeSSAONoise();
//
//    // render graph
//    void BuildRenderGraph(Int32 _width, Int32 _height, const Environment& _env, const Ref<Scene>& _pScene);
//    bool CheckEnvironmentDiff(const Environment& _env) const;
//
//    // shaders
//    ShaderHandle pbrVsh                       = kInvalidHandle;
//    ShaderHandle fullscreenVsh                = kInvalidHandle;
//    ShaderHandle pbrLightVolumeVsh            = kInvalidHandle;
//    ShaderHandle pbrEnvironmentLIghtVolumeVsh = kInvalidHandle;
//    ShaderHandle csmVsh                       = kInvalidHandle;
//    ShaderHandle osmVsh                       = kInvalidHandle;
//    ShaderHandle shadowMappingVsh             = kInvalidHandle;
//    ShaderHandle skyboxVsh                    = kInvalidHandle;
//    ShaderHandle csmLevel3Gsh                 = kInvalidHandle;
//    ShaderHandle osmGsh                       = kInvalidHandle;
//    ShaderHandle pbrGBufferPsh                = kInvalidHandle;
//    ShaderHandle pbrLightingPsh               = kInvalidHandle;
//    ShaderHandle pbrLightVolumePsh            = kInvalidHandle;
//    ShaderHandle pbrEnvironmentLightVolumePsh = kInvalidHandle;
//    ShaderHandle csmPsh                       = kInvalidHandle;
//    ShaderHandle osmPsh                       = kInvalidHandle;
//    ShaderHandle shadowMappingPsh             = kInvalidHandle;
//    ShaderHandle skyboxPsh                    = kInvalidHandle;
//    ShaderHandle ssaoPsh                      = kInvalidHandle;
//    ShaderHandle ssaoBlurXPsh                 = kInvalidHandle;
//    ShaderHandle ssaoBlurYPsh                 = kInvalidHandle;
//    ShaderHandle bloomUpPsh                   = kInvalidHandle;
//    ShaderHandle bloomDownPsh                 = kInvalidHandle;
//    ShaderHandle bloomCombinePsh              = kInvalidHandle;
//    ShaderHandle toneMappingPsh               = kInvalidHandle;
//    ShaderHandle fxaaVeryLowPsh               = kInvalidHandle;
//    ShaderHandle fxaaLowPsh                   = kInvalidHandle;
//    ShaderHandle fxaaMediumPsh                = kInvalidHandle;
//    ShaderHandle fxaaHighPsh                  = kInvalidHandle;
//    ShaderHandle fxaaVeryHighPsh              = kInvalidHandle;
//    ShaderHandle fxaaUltraPsh                 = kInvalidHandle;
//    ShaderHandle fogPsh                       = kInvalidHandle;
//    ShaderHandle passTexhroughPsh             = kInvalidHandle;
//
//    // programs
//    ProgramHandle pbrGBufferProgram                = kInvalidHandle;
//    ProgramHandle pbrLightingProgram               = kInvalidHandle;
//    ProgramHandle pbrLightVolumeProgram            = kInvalidHandle;
//    ProgramHandle pbrEnvironmentLightVolumeProgram = kInvalidHandle;
//    ProgramHandle csmLevel3CasterProgram           = kInvalidHandle;
//    ProgramHandle osmCasterProgram                 = kInvalidHandle;
//    ProgramHandle shadowCasterProgram              = kInvalidHandle;
//    ProgramHandle skyboxProgram                    = kInvalidHandle;
//    ProgramHandle ssaoProgram                      = kInvalidHandle;
//    ProgramHandle ssaoBlurXProgram                 = kInvalidHandle;
//    ProgramHandle ssaoBlurYProgram                 = kInvalidHandle;
//    ProgramHandle bloomUpProgram                   = kInvalidHandle;
//    ProgramHandle bloomDownProgram                 = kInvalidHandle;
//    ProgramHandle bloomCombineProgram              = kInvalidHandle;
//    ProgramHandle toneMappingProgram               = kInvalidHandle;
//    ProgramHandle fxaaVeryLowProgram               = kInvalidHandle;
//    ProgramHandle fxaaLowProgram                   = kInvalidHandle;
//    ProgramHandle fxaaMediumProgram                = kInvalidHandle;
//    ProgramHandle fxaaHighProgram                  = kInvalidHandle;
//    ProgramHandle fxaaVeryHighProgram              = kInvalidHandle;
//    ProgramHandle fxaaUltraProgram                 = kInvalidHandle;
//    ProgramHandle fogProgram                       = kInvalidHandle;
//    ProgramHandle passTexhroughProgram             = kInvalidHandle;
//
//    // cbuffer
//    ConstantBufferHandle transformCbh    = kInvalidHandle;
//    ConstantBufferHandle cameraCbh       = kInvalidHandle;
//    ConstantBufferHandle materialCbh     = kInvalidHandle;
//    ConstantBufferHandle lightCbh        = kInvalidHandle;
//    ConstantBufferHandle environmentCbh  = kInvalidHandle;
//    ConstantBufferHandle shadowCasterCbh = kInvalidHandle;
//    ConstantBufferHandle csmCasterCbh    = kInvalidHandle;
//    ConstantBufferHandle osmCasterCbh    = kInvalidHandle;
//    ConstantBufferHandle ssaoCbh         = kInvalidHandle;
//    ConstantBufferHandle screenCbh       = kInvalidHandle;
//
//    // meshes
//    Mesh fullScreenTri            = {};
//    Mesh lightVolumeFullScreenTri = {};
//    Mesh lightVolumeUnitSphere    = {};
//    Mesh lightVolumeUnitCone      = {};
//    Mesh skyboxCube               = {};
//
//    // samplers
//    SamplerState linearWrapSampler        = {};
//    SamplerState anisotropicSampler       = {};
//    SamplerState linearClampSampler       = {};
//    SamplerState pointClampSampler        = {};
//    SamplerState shadowLinearClampSampler = {};
//
//    // states
//    RenderState depthTestState   = {};
//    RenderState lightVolumeState = {};
//    RenderState skyboxState      = {};
//
//    // misc
//    TextureHandle ssaoNoiseTexture = kInvalidHandle;
//
//    // render graph
//    RenderGraph renderGraph = {};
//
//    // scene resource
//    const Environment* pEnv   = nullptr;
//    Int32              width  = 0;
//    Int32              height = 0;
//
//    bool              bDirLightEnabled       = false;
//    bool              bDirLightShadowEnabled = false;
//    Int32             numBloomPasses         = -1;
//    Ref<Texture> pIrrdianceMap          = nullptr;
//    Ref<Texture> pSpecularMap           = nullptr;
//    Ref<Texture> pBrdfLUTMap            = nullptr;
//    Ref<Texture> pSkyboxTeX         = nullptr;
//    bool              bSSAOEnabled           = false;
//    bool              bFogEnabled            = false;
//    bool              bToneMappingEnabled    = false;
//    bool              bBloomEnabled          = false;
//    bool              bFXAAEnabled           = false;
//
//    TextureHandle hdrTexh             = kInvalidHandle;
//    TextureHandle depthTexh           = kInvalidHandle;
//    TextureHandle gbufferAlbedoTexh   = kInvalidHandle;
//    TextureHandle gbufferMaterialTexh = kInvalidHandle;
//    TextureHandle gbufferNormalTexh   = kInvalidHandle;
//    TextureHandle gbufferLightingTexh = kInvalidHandle;
//
//    FrameBufferHandle hdrNoDepthFbh = kInvalidHandle;
//    FrameBufferHandle hdrDepthFbh   = kInvalidHandle;
//
//}* g_basicRendererCtx = nullptr;
//
//void BasicRendererContext::UploadTransform(
//    const Matrix& _world) const
//{
//    CB_TRANSFORM cb;
//    cb.cb_world         = _world.Transpose();
//    cb.cb_worldInvTrans = _world.Invert();
//    gfx.UploadBuffer(transformCbh, cb);
//}
//
//void BasicRendererContext::UploadCamera(
//    const Vec3  _eye,
//    const Matrix& _view,
//    const Matrix& _proj) const
//{
//    // camera buffer
//    CB_CAMERA cb;
//    cb.cb_view        = _view.Transpose();
//    cb.cb_proj        = _proj.Transpose();
//    cb.cb_viewProjInv = (cb.cb_proj * cb.cb_view).Invert();   // (_view * _proj).GetInvert().Transpose() == (_proj.Transpose() * _view.Transpose()).GetInvert()
//    cb.cb_eye         = _eye;
//    gfx.UploadBuffer(g_basicRendererCtx->cameraCbh, cb);
//}
//
//void BasicRendererContext::UploadEnvironment(
//    const Environment& _env) const
//{
//    constexpr UInt32 kToneMappintTable[] = {
//        JAM_ENVIRONMENT_TONE_MAPPING_TYPE_LINEAR,
//        JAM_ENVIRONMENT_TONE_MAPPING_TYPE_REINHARD,
//        JAM_ENVIRONMENT_TONE_MAPPING_TYPE_LUMA_BASED_REINHARD,
//        JAM_ENVIRONMENT_TONE_MAPPING_TYPE_WHITE_PRESERVING_REINHARD,
//        JAM_ENVIRONMENT_TONE_MAPPING_TYPE_ROMB_DA_HOUSE,
//        JAM_ENVIRONMENT_TONE_MAPPING_TYPE_UNCHARTED2,
//        JAM_ENVIRONMENT_TONE_MAPPING_TYPE_FILMIC,
//        JAM_ENVIRONMENT_TONE_MAPPING_TYPE_NONE
//    };
//
//    const DirectionalLightData&   dirLight    = _env.GetDirectionalLight();
//    const ImageBasedLightingData& ibl         = _env.GetIBL();
//    const SkyboxData&             skybox      = _env.GetSkybox();
//    const SSAOData&               ssao        = _env.GetSSAO();
//    const FogData&                fog         = _env.GetFog();
//    const ToneMappingData&        toneMapping = _env.GetToneMapping();
//    const BloomData&              bloom       = _env.GetBloom();
//    const EnvironmentMisc&        misc        = _env.GetMisc();
//
//    CB_ENVIRONMENT cb;
//    cb.cb_envFlags = JAM_ENVIRONMENT_FLAGS_NONE;
//
//    // directional Light
//    {
//        cb.cb_dirLightIntensity = dirLight.intensity;
//        cb.cb_dirLightDirection = dirLight.direction;
//        cb.cb_dirLightRadiance  = dirLight.radiance;
//    }
//
//    // shadow
//    {
//        if (dirLight.bShadowEnabled)
//        {
//            cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_DIRLIGHT_SHADOW;
//        }
//
//        // shadow cascade ranges
//        cb.cb_cascadeShadowDistances.x = dirLight.cascadeDistances[0];
//        cb.cb_cascadeShadowDistances.y = dirLight.cascadeDistances[1];
//        cb.cb_cascadeShadowDistances.z = dirLight.cascadeDistances[2];
//
//        // shadow bias
//        cb.cb_shadowBias = misc.shadowBias;
//    }
//
//    // ibl
//    {
//        cb.cb_irradianceMapIntensity = ibl.irradianceTexIntensity;
//        cb.cb_specularMapIntensity   = ibl.specularTexIntensity;
//
//        if (ibl.pSpecularTex)
//        {
//            const TextureHandle texh = ibl.pSpecularTex->GetHandle();
//            const TextureInfo   info = gfx.GetTextureInfo(texh);
//            cb.cb_specularMapNumMips = info.numMips;
//        }
//        else
//        {
//            cb.cb_specularMapNumMips = 0;
//        }
//    }
//
//    // skybox
//    {
//        cb.cb_skyBoxSolidColor = skybox.solidColor;
//    }
//
//    // ssao
//    {
//        cb.cb_ssaoRadius = ssao.radius;
//        cb.cb_ssaoBias   = ssao.bias;
//    }
//
//    // fog
//    {
//        cb.cb_fogColor             = fog.radiance;
//        cb.cb_fogDensity           = fog.density;
//        cb.cb_fogFallOffStart      = fog.fallOffStart;
//        cb.cb_fogFallOffEnd        = fog.fallOffEnd;
//        cb.cb_fogHeightAttenuation = fog.heightAttenuation;
//    }
//
//    // tone mapping
//    {
//        cb.cb_toneMappingType = kToneMappintTable[UnderlyingFrom<eToneMapping>(toneMapping.type)];
//        cb.cb_exposure        = toneMapping.exposure;
//        cb.cb_gamma           = toneMapping.gamma;
//    }
//
//    // bloom
//    {
//        cb.cb_bloomBlurRadius = bloom.blurRadius;
//        cb.cb_bloomStrength   = bloom.bloomStrength;
//    }
//
//    // environment flags
//    if (ssao.bEnabled)
//    {
//        cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SSAO;
//    }
//
//    if (skybox.type == eSkybox::CubeMap)
//    {
//        cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SKYBOX_CUBEMAP;
//    }
//    else if (skybox.type == eSkybox::SolidColor)
//    {
//        cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SKYBOX_SOLID_COLOR;
//    }
//
//    gfx.UploadBuffer(g_basicRendererCtx->environmentCbh, cb);
//}
//
//void BasicRendererContext::UploadScreen(
//    const Int32 _width,
//    const Int32 _height) const
//{
//    CB_SCREEN cb;
//    cb.cb_screenWidth  = static_cast<float>(_width);
//    cb.cb_screenHeight = static_cast<float>(_height);
//    cb.cb_texelX       = 1.f / static_cast<float>(_width);
//    cb.cb_texelY       = 1.f / static_cast<float>(_height);
//    gfx.UploadBuffer(screenCbh, cb);
//}
//
//void BasicRendererContext::UploadMaterial(
//    const Ref<Material>& _material) const
//{
//    constexpr UInt32 kMetaliicChannelTable[] = {
//        JAM_MATERIAL_FLAGS_METALLIC_R,   // red
//        JAM_MATERIAL_FLAGS_METALLIC_G,   // green
//        JAM_MATERIAL_FLAGS_METALLIC_B,   // blue
//        JAM_MATERIAL_FLAGS_METALLIC_R    // alpha
//    };
//
//    constexpr UInt32 kRoughnessChannelTable[] = {
//        JAM_MATERIAL_FLAGS_ROUGHNESS_R,   // red
//        JAM_MATERIAL_FLAGS_ROUGHNESS_G,   // green
//        JAM_MATERIAL_FLAGS_ROUGHNESS_B,   // blue
//        JAM_MATERIAL_FLAGS_ROUGHNESS_R    // alpha
//    };
//
//    const MaterialParams&          params   = _material->GetParams();
//    const MaterialTextures&        texes = _material->GetTextures();
//    const MaterialTextureSettings& settings = _material->GetTextureSettings();
//
//    CB_MATERIAL cb               = {};
//    cb.cb_ambient                = params.albedo;
//    cb.cb_diffuse                = params.diffuse;
//    cb.cb_specular               = params.specular;
//    cb.cb_shininess              = params.shininess;
//    cb.cb_albedo                 = params.albedo;
//    cb.cb_roughness              = params.roughness;
//    cb.cb_metallic               = params.metallic;
//    cb.cb_emissive               = params.emissive;
//    cb.cb_emissiveIntensity      = params.emissiveIntensity;
//    cb.cb_lightTextureIntencity  = settings.lightMapIntensity;
//    cb.cb_displacementMapScaling = settings.vertexDisplacementMapScale;
//    cb.cb_materialFlags          = 0;
//
//    if (const Ref<Texture>& pTexture = texes.Get(eMaterialTexture::Albedo))
//    {
//        pTexture->Bind(eShader::PixelShader, t_albedoSlot);
//        cb.cb_materialFlags |= JAM_MATERIAL_FLAGS_ALBEDO;
//    }
//
//    if (const Ref<Texture>& pTexture = texes.Get(eMaterialTexture::Normal))
//    {
//        pTexture->Bind(eShader::PixelShader, t_normalSlot);
//        cb.cb_materialFlags |= JAM_MATERIAL_FLAGS_NORMAL;
//    }
//
//    if (const Ref<Texture>& pTexture = texes.Get(eMaterialTexture::Metallic))
//    {
//        const eTextureChannel channel = settings.metallicMapChannel;
//        pTexture->Bind(eShader::PixelShader, t_metallicSlot);
//        cb.cb_materialFlags |= kMetaliicChannelTable[UnderlyingFrom<eTextureChannel>(channel)];
//    }
//
//    if (const Ref<Texture>& pTexture = texes.Get(eMaterialTexture::Roughness))
//    {
//        const eTextureChannel channel = settings.roughnessMapChannel;
//        pTexture->Bind(eShader::PixelShader, t_roughnessSlot);
//        cb.cb_materialFlags |= kRoughnessChannelTable[UnderlyingFrom<eTextureChannel>(channel)];
//    }
//
//    if (const Ref<Texture>& pTexture = texes.Get(eMaterialTexture::Ao))
//    {
//        pTexture->Bind(eShader::PixelShader, t_aoSlot);
//        cb.cb_materialFlags |= JAM_MATERIAL_FLAGS_AO;
//    }
//
//    if (const Ref<Texture>& pTexture = texes.Get(eMaterialTexture::Emissive))
//    {
//        pTexture->Bind(eShader::PixelShader, t_emissiveSlot);
//        cb.cb_materialFlags |= JAM_MATERIAL_FLAGS_EMISSIVE;
//    }
//
//    if (const Ref<Texture>& pTexture = texes.Get(eMaterialTexture::VertexDisplacement))
//    {
//        pTexture->Bind(eShader::VertexShader, t_displacementMapSlot);
//        cb.cb_materialFlags |= JAM_MATERIAL_FLAGS_VERTEX_DISPLACEMENT_MAP;
//    }
//
//    if (const Ref<Texture>& pTexture = texes.Get(eMaterialTexture::Light))
//    {
//        pTexture->Bind(eShader::PixelShader, t_lightmapSlot);
//        cb.cb_materialFlags |= JAM_MATERIAL_FLAGS_LIGHT_MAP;
//    }
//
//    if (settings.bNormalMapFlipedY)
//    {
//        cb.cb_materialFlags |= JAM_MATERIAL_FLAGS_FLIP_Y_NORMAL;
//    }
//
//    if (params.bAlphaCutout)
//    {
//        cb.cb_materialFlags |= JAM_MATERIAL_FLAGS_ALPHA_CUTOUT;
//    }
//
//    gfx.UploadBuffer(materialCbh, cb);
//}
//
//void BasicRendererContext::UploadPointLight(
//    const Vec3  _radiance,
//    const float _lightIntensity,
//    const Vec3  _pos,
//    const float _fallOffStart,
//    const float _fallOffEnd,
//    const bool  _bShadow) const
//{
//    CB_LIGHT cb                 = {};
//    cb.cb_lightType             = JAM_LIGHT_TYPE_POINT;
//    cb.cb_lightPos              = _pos;
//    cb.cb_lightDir              = Vec3::kForward;
//    cb.cb_lightRadiance         = _radiance;
//    cb.cb_lightIntensity        = _lightIntensity;
//    cb.cb_lightFallOffStart     = _fallOffStart;
//    cb.cb_lightFallOffEnd       = _fallOffEnd;
//    cb.cb_lightInnerConeHalfRad = 0;
//    cb.cb_lightOuterConeHalfRad = 0;
//    cb.cb_bShadowMapping        = _bShadow ? JAM_TRUE : JAM_FALSE;
//    gfx.UploadBuffer(lightCbh, cb);
//}
//
//void BasicRendererContext::UploadSpotLight(
//    const Vec3  _radiance,
//    const float _lightIntensity,
//    const Vec3  _pos,
//    const Vec3  _dir,
//    const float _fallOffStart,
//    const float _fallOffEnd,
//    const float _innerConeAngle,
//    const float _outerConeAngle,
//    const bool  _bShadow) const
//{
//    CB_LIGHT cb                 = {};
//    cb.cb_lightType             = JAM_LIGHT_TYPE_POINT;
//    cb.cb_lightPos              = _pos;
//    cb.cb_lightDir              = _dir;
//    cb.cb_lightRadiance         = _radiance;
//    cb.cb_lightIntensity        = _lightIntensity;
//    cb.cb_lightFallOffStart     = _fallOffStart;
//    cb.cb_lightFallOffEnd       = _fallOffEnd;
//    cb.cb_lightInnerConeHalfRad = _innerConeAngle;
//    cb.cb_lightOuterConeHalfRad = _outerConeAngle;
//    cb.cb_bShadowMapping        = _bShadow ? JAM_TRUE : JAM_FALSE;
//    gfx.UploadBuffer(lightCbh, cb);
//}
//
//TextureHandle BasicRendererContext::CreateTexture2D(
//    const Int32           _width,
//    const Int32           _height,
//    const eTextureFormat  _format,
//    const eAttachment _type,
//    const StringView      _name) const
//{
//    Flags<eTextureCreateFlags> flags = eTextureCreateFlags::None;
//
//    if (_type == eAttachment::RenderTarget)
//    {
//        flags |= eTextureCreateFlags::RenderTarget;
//    }
//
//    if (_type == eAttachment::DepthStencil)
//    {
//        flags |= eTextureCreateFlags::DepthStencil;
//    }
//
//    const TextureHandle texh = gfx.CreateTexture2D(_width, _height, _format, 1, flags);
//    if (!_name.IsEmpty())
//    {
//        gfx.SetTextureName(texh, _name);
//    }
//    return texh;
//}
//
//TextureHandle BasicRendererContext::CreateTextureCube(
//    const Int32           _width,
//    const Int32           _height,
//    const eTextureFormat  _format,
//    const eAttachment _type,
//    const StringView      _name) const
//{
//    Flags<eTextureCreateFlags> flags = eTextureCreateFlags::None;
//
//    if (_type == eAttachment::RenderTarget)
//    {
//        flags |= eTextureCreateFlags::RenderTarget;
//    }
//
//    if (_type == eAttachment::DepthStencil)
//    {
//        flags |= eTextureCreateFlags::DepthStencil;
//    }
//
//    const TextureHandle texh = gfx.CreateTextureCube(_width, _height, _format, 1, flags);
//    if (!_name.IsEmpty())
//    {
//        gfx.SetTextureName(texh, _name);
//    }
//    return texh;
//}
//
//TextureHandle BasicRendererContext::CreateTextureArray(
//    const Int32           _width,
//    const Int32           _height,
//    const Int32           _numLayers,
//    const eTextureFormat  _format,
//    const eAttachment _type,
//    const StringView      _name) const
//{
//    Flags<eTextureCreateFlags> flags = eTextureCreateFlags::None;
//
//    if (_type == eAttachment::RenderTarget)
//    {
//        flags |= eTextureCreateFlags::RenderTarget;
//    }
//
//    if (_type == eAttachment::DepthStencil)
//    {
//        flags |= eTextureCreateFlags::DepthStencil;
//    }
//
//    const TextureHandle texh = gfx.CreateTexture2D(_width, _height, _format, _numLayers, flags);
//    if (!_name.IsEmpty())
//    {
//        gfx.SetTextureName(texh, _name);
//    }
//    return texh;
//}
//
//FrameBufferHandle BasicRendererContext::CreateFrameBuffer(
//    const Span<const TextureHandle> _attachment,
//    const bool                      _bDestroyTexture,
//    const StringView                _name) const
//{
//    const FrameBufferHandle fbh = gfx.CreateFrameBuffer(_attachment, _bDestroyTexture);
//    if (!_name.IsEmpty())
//    {
//        gfx.SetFrameBufferName(fbh, _name);
//    }
//    return fbh;
//}
//
//FrameBufferHandle BasicRendererContext::CreateFrameBuffer(
//    const TextureHandle _texh,
//    const bool          _bDestroyTexture,
//    const StringView    _name) const
//{
//    const FrameBufferHandle fbh = gfx.CreateFrameBuffer(_texh, _bDestroyTexture);
//    if (!_name.IsEmpty())
//    {
//        gfx.SetFrameBufferName(fbh, _name);
//    }
//    return fbh;
//}
//
//FrameBufferHandle BasicRendererContext::CreateFrameBuffer(
//    const Int32          _width,
//    const Int32          _height,
//    const eTextureFormat _format,
//    const StringView     _name) const
//{
//    const FrameBufferHandle fbh = gfx.CreateFrameBuffer(_width, _height, _format);
//    if (!_name.IsEmpty())
//    {
//        gfx.SetFrameBufferName(fbh, _name);
//    }
//    return fbh;
//}
//
//void BasicRendererContext::Submit(
//    const Ref<Mesh>&     _mesh,
//    const Ref<Material>& _material,
//    const Matrix&          _world) const
//{
//    UploadMaterial(_material);
//    UploadTransform(_world);
//    _mesh->Submit();
//}
//
//void BasicRendererContext::SubmitFullScreenTri() const
//{
//    fullScreenTri.Submit();
//}
//
//void BasicRendererContext::SubmitSkyboxCube() const
//{
//    g_basicRendererCtx->skyboxCube.Submit();
//}
//
//void BasicRendererContext::BakeSSAOKernel() const
//{
//    Vec4 kernel[JAM_SSAO_KERNEL_SIZE];
//    for (Int32 i = 0; i < JAM_SSAO_KERNEL_SIZE; ++i)
//    {
//        // 반구
//        Vec4 sample = Normalized(Vec4 { MakeRandom(-1.f, 1.f), MakeRandom(-1.f, 1.f), MakeRandom(0.f, 1.f), 0.f });
//        sample *= MakeRandom(0.f, 1.f);
//
//        // 가까운 샘플에 더 집중
//        const float t = Square(static_cast<float>(i) / static_cast<float>(JAM_SSAO_KERNEL_SIZE));
//        sample *= Lerp(0.1f, 1.f, t);
//
//        kernel[i] = sample;
//    }
//
//    gfx.UploadBuffer(ssaoCbh, kernel);
//}
//
//void BasicRendererContext::BakeSSAONoise()
//{
//    JAM_DESTROY_TEXTURE(ssaoNoiseTexture);
//
//    constexpr Int32 kLength = 4;
//
//    Float16 noiseData[4 * kLength * kLength];
//    for (Int32 i = 0; i < kLength * kLength; ++i)
//    {
//        const Int32 index    = i * 4;
//        noiseData[index + 0] = ToFloat16(MakeRandom(-1.f, 1.f));
//        noiseData[index + 1] = ToFloat16(MakeRandom(-1.f, 1.f));
//        noiseData[index + 2] = ToFloat16(0.f);
//        noiseData[index + 3] = ToFloat16(0.f);
//    }
//
//    const CMemoryBlock mem[] = { noiseData };
//    ssaoNoiseTexture     = gfx.CreateTexture2D(kLength, kLength, eTextureFormat::RGBA16_Float, 1, eTextureCreateFlags::None, mem);
//}
//
//void BasicRendererContext::BuildRenderGraph(
//    Int32              _width,
//    Int32              _height,
//    const Environment& _env,
//    const Ref<Scene>&  _pScene)
//{
//    auto gBufferExecutor = [_pScene]
//    {
//        for (const auto& [_, sc, rc]: _pScene->Each<SceneComponent, RenderableComponent>())
//        {
//            const Ref<Material>& pMaterial  = rc.renderable.pMaterial;
//            const Ref<Mesh>&     pMesh = rc.renderable.pMesh;
//            if (pMaterial && pMesh)
//            {
//                const Transform& wt = sc.node.GetWorldTransform();
//                g_basicRendererCtx->Submit(pMesh, pMaterial, wt.GetMatrix());
//            }
//        }
//    };
//
//    auto bloomFullScreenTriExecutor = [_width, _height]
//    {
//        g_basicRendererCtx->UploadScreen(_width, _height);
//        g_basicRendererCtx->SubmitFullScreenTri();
//    };
//
//    ////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    JAM_FUNCTION_PROFILE();
//    JAM_DESTROY_TEXTURE(hdrTexh);
//    JAM_DESTROY_TEXTURE(depthTexh);
//    JAM_DESTROY_TEXTURE(gbufferAlbedoTexh);
//    JAM_DESTROY_TEXTURE(gbufferMaterialTexh);
//    JAM_DESTROY_TEXTURE(gbufferNormalTexh);
//    JAM_DESTROY_TEXTURE(gbufferLightingTexh);
//    JAM_DESTROY_FRAME_BUFFER(hdrNoDepthFbh);
//    JAM_DESTROY_FRAME_BUFFER(hdrDepthFbh);
//    renderGraph.Reset();
//
//    // var
//    RenderGraph&   graph = renderGraph;
//    const Viewport vp    = { static_cast<UInt16>(_width), static_cast<UInt16>(_height) };
//
//    const DirectionalLightData&   dirLight    = _env.GetDirectionalLight();
//    const ImageBasedLightingData& ibl         = _env.GetIBL();
//    const SkyboxData&             skybox      = _env.GetSkybox();
//    const SSAOData&               ssao        = _env.GetSSAO();
//    const FogData&                fog         = _env.GetFog();
//    const ToneMappingData&        toneMapping = _env.GetToneMapping();
//    const BloomData&              bloom       = _env.GetBloom();
//    const FXAAData&               fxaa        = _env.GetFXAA();
//
//    // update current context
//    width                  = _width;
//    height                 = _height;
//    pEnv                   = &_env;
//    bDirLightEnabled       = dirLight.bEnabled;
//    bDirLightShadowEnabled = dirLight.bShadowEnabled;
//    pIrrdianceMap          = ibl.pIrradianceTex;
//    pSpecularMap           = ibl.pSpecularTex;
//    pBrdfLUTMap            = ibl.pBrdfLUTTex;
//    pSkyboxTeX         = skybox.pSkyboxTeX;
//    bSSAOEnabled           = ssao.bEnabled;
//    bFogEnabled            = fog.bEnabled;
//    bToneMappingEnabled    = toneMapping.bEnabled;
//    bBloomEnabled          = bloom.bEnabled;
//    numBloomPasses         = bloom.numBlurPasses;
//    bFXAAEnabled           = fxaa.bEnabled;
//
//    // scene resources
//    hdrTexh             = CreateTexture2D(width, height, eTextureFormat::RGBA16_Float, eAttachment::RenderTarget, "HDR Texture");
//    depthTexh           = CreateTexture2D(width, height, eTextureFormat::D24_UNorm_S8_UInt, eAttachment::DepthStencil, "Depth Texture");
//    gbufferAlbedoTexh   = CreateTexture2D(width, height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget, "GBuffer Albedo Texture");
//    gbufferMaterialTexh = CreateTexture2D(width, height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget, "GBuffer Material Texture");
//    gbufferNormalTexh   = CreateTexture2D(width, height, eTextureFormat::RGB10A2_UNorm, eAttachment::RenderTarget, "GBuffer Normal Texture");
//    gbufferLightingTexh = CreateTexture2D(width, height, eTextureFormat::RG11B10_Float, eAttachment::RenderTarget, " GBuffer Lighting Texture");
//
//    // will be created if ssao is enabled
//    TextureHandle ssaoTexh = kInvalidHandle;
//
//    // hdr fbh
//    {
//        const TextureHandle attachment[] = { hdrTexh, depthTexh };
//        hdrNoDepthFbh                    = CreateFrameBuffer(hdrTexh, false, "HDR FrameBuffer");
//        hdrDepthFbh                      = CreateFrameBuffer(attachment, false, "HDR + Depth FrameBuffer");
//    }
//
//    // gbuffer geometry pass
//    {
//        const TextureHandle     gbufferAttachments[] = { gbufferAlbedoTexh, gbufferMaterialTexh, gbufferNormalTexh, gbufferLightingTexh, depthTexh };
//        const FrameBufferHandle gbufferFbh           = CreateFrameBuffer(gbufferAttachments, false, "GBuffer FrameBuffer");
//
//        RenderPass pass = {};
//        pass.SetProgram(pbrGBufferProgram);
//        pass.AddSampler(linearWrapSampler, eShader::VertexShader, s_linearWrapSlot);
//        pass.AddSampler(linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
//        pass.AddConstantBuffer(transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT);
//        pass.AddConstantBuffer(cameraCbh, eShader::VertexShader, CB_CAMERA_SLOT);
//        pass.AddConstantBuffer(materialCbh, eShader::VertexShader, CB_MATERIAL_SLOT);
//        pass.AddConstantBuffer(materialCbh, eShader::PixelShader, CB_MATERIAL_SLOT);
//        pass.AddConstantBuffer(environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT);
//        pass.SetFrameBuffer(gbufferFbh, true);
//        pass.SetClearRenderTargetCommand(RGBA::kBlack, 0);
//        pass.SetClearRenderTargetCommand(RGBA::kBlack, 1);
//        pass.SetClearRenderTargetCommand(RGBA::kBlack, 2);
//        pass.SetClearDepthStencilCommand(true, false, 1.f, 0);
//        pass.SetViewport(vp);
//        pass.SetRenderState(depthTestState);
//        pass.SetBlendState(BlendState::kOpaque, 0);
//        pass.SetStencil(StencilState::kDisable, StencilState::kDisable, 0);
//        // pass.SetExecutor(EmplacedDelegate<&GBufferExecutor::Summit, GBufferExecutor>(_pScene));
//        // auto test = EmplacedDelegate<&GBufferExecutor::Summit, GBufferExecutor>(nullptr);
//        // todo
//
//        graph.AddPass(std::move(pass));
//    }
//
//    // ssao pass
//    if (bSSAOEnabled)
//    {
//        // ssao
//        const TextureHandle     ssaoTexh0 = CreateTexture2D(width, height, eTextureFormat::R8_UNorm, eAttachment::RenderTarget, "SSAO0 Texture");
//        const FrameBufferHandle ssaoFbh0  = CreateFrameBuffer(ssaoTexh0, true, "SSAO0 FrameBuffer");
//
//        RenderPass pass0 = {};
//        pass0.SetProgram(ssaoProgram);
//        pass0.AddSampler(pointClampSampler, eShader::PixelShader, s_pointClampSlot);
//        pass0.AddSampler(linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
//        pass0.AddConstantBuffer(cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT);
//        pass0.AddConstantBuffer(ssaoCbh, eShader::PixelShader, CB_SSAO_KERNEL_SLOT);
//        pass0.AddConstantBuffer(environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT);
//        pass0.AddConstantBuffer(screenCbh, eShader::PixelShader, CB_SCREEN_SLOT);
//        pass0.AddTexture(gbufferNormalTexh, eShader::PixelShader, t_gBufferNormalSlot);
//        pass0.AddTexture(depthTexh, eShader::PixelShader, t_depthSlot);
//        pass0.AddTexture(ssaoNoiseTexture, eShader::PixelShader, t_ssaoNoiseSlot);
//        pass0.SetFrameBuffer(ssaoFbh0, true);
//        pass0.SetViewport(vp);
//        pass0.SetRenderState(RenderState::kDefault);
//        pass0.SetBlendState(BlendState::kOpaque, 0);
//        pass0.SetStencil(StencilState::kDisable, StencilState::kDisable, 0);
//        pass0.SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
//
//        // horizontal blur
//        const TextureHandle     ssaoTexh1 = CreateTexture2D(width, height, eTextureFormat::R8_UNorm, eAttachment::RenderTarget, "SSAO H-Blur Texture");
//        const FrameBufferHandle ssaoFbh1  = CreateFrameBuffer(ssaoTexh1, true, "SSAO H-Blur FrameBuffer");
//
//        RenderPass pass1 = {};
//        pass1.SetProgram(ssaoBlurXProgram);
//        pass1.AddConstantBuffer(screenCbh, eShader::PixelShader, CB_SCREEN_SLOT);
//        pass1.AddTexture(ssaoTexh0, eShader::PixelShader, t_ssaoSlot);
//        pass1.AddSampler(linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
//        pass1.SetFrameBuffer(ssaoFbh1, true);
//        pass1.SetViewport(vp);
//        pass1.SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
//
//        // vertical blur
//        const TextureHandle     ssaoTexh2 = CreateTexture2D(width, height, eTextureFormat::R8_UNorm, eAttachment::RenderTarget, "SSAO Texture");
//        const FrameBufferHandle ssaoFbh2  = CreateFrameBuffer(ssaoTexh2, true, "SSAO V-Blur FrameBuffer");
//
//        RenderPass pass2 = {};
//        pass2.SetProgram(ssaoBlurYProgram);
//        pass2.AddConstantBuffer(screenCbh, eShader::PixelShader, CB_SCREEN_SLOT);
//        pass2.AddTexture(ssaoTexh1, eShader::PixelShader, t_ssaoSlot);
//        pass2.AddSampler(linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
//        pass2.SetFrameBuffer(ssaoFbh2, true);
//        pass2.SetViewport(vp);
//        pass2.SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
//
//        // final ssao texture
//        ssaoTexh = ssaoTexh2;
//
//        graph.AddPass(std::move(pass0));
//        graph.AddPass(std::move(pass1));
//        graph.AddPass(std::move(pass2));
//    }
//
//    // gbuffer lighting pass
//    {
//        if (!pIrrdianceMap)
//        {
//           JAM_LOG_WARN("IBL irradiance map is not set in environment, the lighting may look incorrect.");
//        }
//
//        if (!pSpecularMap)
//        {
//           JAM_LOG_WARN("IBL specular map is not set in environment, the lighting may look incorrect.");
//        }
//
//        if (!pBrdfLUTMap)
//        {
//           JAM_LOG_WARN("IBL BRDF LUT is not set in environment, the lighting may look incorrect.");
//        }
//
//        if (bSSAOEnabled && !ssaoTexh)
//        {
//           JAM_LOG_WARN("SSAO is enabled in environment but SSAO texture is not generated, the ambient occlusion effect will not be applied.");
//        }
//
//        RenderPass pass = {};
//        pass.SetProgram(pbrLightingProgram);
//        pass.AddSampler(linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
//        pass.AddSampler(linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        pass.AddSampler(pointClampSampler, eShader::PixelShader, s_pointClampSlot);
//        pass.AddConstantBuffer(cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT);
//        pass.AddConstantBuffer(environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT);
//        pass.AddConstantBuffer(screenCbh, eShader::PixelShader, CB_SCREEN_SLOT);
//        pass.AddTexture(gbufferAlbedoTexh, eShader::PixelShader, t_gBufferAlbedoSlot);
//        pass.AddTexture(gbufferMaterialTexh, eShader::PixelShader, t_gBufferMaterialSlot);
//        pass.AddTexture(gbufferNormalTexh, eShader::PixelShader, t_gBufferNormalSlot);
//        pass.AddTexture(gbufferLightingTexh, eShader::PixelShader, t_gBufferLightingSlot);
//        pass.AddTexture(depthTexh, eShader::PixelShader, t_depthSlot);
//        pass.SetFrameBuffer(hdrNoDepthFbh);
//        pass.SetViewport(vp);
//        pass.SetRenderState(RenderState::kDefault);
//        pass.SetBlendState(BlendState::kOpaque, 0);
//        pass.SetStencil(StencilState::kDisable, StencilState::kDisable, 0);
//        pass.SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
//
//        if (pIrrdianceMap)
//        {
//            pass.AddTexture(pIrrdianceMap->GetHandle(), eShader::PixelShader, t_irradianceSlot);
//        }
//        if (pSpecularMap)
//        {
//            pass.AddTexture(pSpecularMap->GetHandle(), eShader::PixelShader, t_specularSlot);
//        }
//        if (pBrdfLUTMap)
//        {
//            pass.AddTexture(pBrdfLUTMap->GetHandle(), eShader::PixelShader, t_brdfSlot);
//        }
//        if (bSSAOEnabled)
//        {
//            pass.AddTexture(ssaoTexh, eShader::PixelShader, t_ssaoSlot);
//        }
//
//        graph.AddPass(std::move(pass));
//    }
//
//    // skybox
//    {
//        if (skybox.type == eSkybox::CubeMap && !pSkyboxTeX)
//        {
//           JAM_LOG_WARN("Skybox type is set to CubeMap in environment but skybox texture is not set, the skybox will not be rendered.");
//        }
//
//        RenderPass pass = {};
//        pass.SetProgram(skyboxProgram);
//        pass.AddSampler(linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        pass.AddConstantBuffer(cameraCbh, eShader::VertexShader, CB_CAMERA_SLOT);
//        pass.AddConstantBuffer(cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT);
//        pass.SetFrameBuffer(hdrDepthFbh);
//        pass.SetViewport(vp);
//        pass.SetRenderState(skyboxState);
//        pass.SetBlendState(BlendState::kOpaque, 0);
//        pass.SetStencil(StencilState::kDisable, StencilState::kDisable, 0);
//        pass.SetExecutor(Bind<&BasicRendererContext::SubmitSkyboxCube>(g_basicRendererCtx));
//
//        if (skybox.type == eSkybox::CubeMap && pSkyboxTeX)
//        {
//            pass.AddTexture(skybox.pSkyboxTeX->GetHandle(), eShader::PixelShader, t_skyboxSlot);
//        }
//
//        graph.AddPass(std::move(pass));
//    }
//
//    // 이하 포스트 프로세스 패스
//
//    TextureHandle inputTexh = hdrTexh;
//
//    // 만약. 포스트 프로세스가 하나도 활성화 되어있지 않다면 패스 스루 패스를 추가한다. (hdr -> ldr 때문)
//    if (!(fog.bEnabled || bloom.bEnabled || fxaa.bEnabled || toneMapping.bEnabled))
//    {
//        // if no post process is enabled.
//        // add pass through pass
//
//        const TextureHandle     texh = CreateTexture2D(width, height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget, "Pass Through Texture");
//        const FrameBufferHandle fbh  = CreateFrameBuffer(texh, true, "Pass Through FrameBuffer");
//
//        RenderPass pass = {};
//        pass.SetProgram(passTexhroughProgram);
//        pass.AddSampler(linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
//        pass.AddTexture(hdrTexh, eShader::PixelShader, t_input0Slot);
//        pass.SetFrameBuffer(fbh, true);
//        pass.SetViewport(vp);
//        pass.SetRenderState(RenderState::kDefault);
//        pass.SetBlendState(BlendState::kOpaque, 0);
//        pass.SetStencil(StencilState::kDisable, StencilState::kDisable, 0);
//        pass.SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
//
//        graph.AddPass(std::move(pass));
//    }
//
//    // fog
//    if (fog.bEnabled)
//    {
//        const TextureHandle     texh = CreateTexture2D(width, height, eTextureFormat::RG11B10_Float, eAttachment::RenderTarget, "Fog Texture");
//        const FrameBufferHandle fbh  = CreateFrameBuffer(texh, true, "Fog FrameBuffer");
//
//        RenderPass pass = {};
//        pass.SetProgram(fogProgram);
//        pass.AddSampler(linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
//        pass.AddSampler(pointClampSampler, eShader::PixelShader, s_pointClampSlot);
//        pass.AddConstantBuffer(cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT);
//        pass.AddConstantBuffer(environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT);
//        pass.AddTexture(depthTexh, eShader::PixelShader, t_depthSlot);
//        pass.AddTexture(inputTexh, eShader::PixelShader, t_input0Slot);
//        pass.SetFrameBuffer(fbh, true);
//        pass.SetViewport(vp);
//        pass.SetRenderState(RenderState::kDefault);
//        pass.SetBlendState(BlendState::kOpaque, 0);
//        pass.SetStencil(StencilState::kDisable, StencilState::kDisable, 0);
//        pass.SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
//
//        graph.AddPass(std::move(pass));
//        inputTexh = texh;
//    }
//
//    //// bloom
//    // if (bloom.bEnabled)
//    //{
//    //     const TextureHandle originalTexh       = inputTexh;
//    //     const Int32         numBlurPassesLimit = CalcNumMips(width, height, 1);
//    //     const Int32         numBlurPasses      = Clamp(bloom.numBlurPasses, 1, numBlurPassesLimit);
//
//    //    // down
//    //    for (Int32 i = 0; i < numBlurPasses; ++i)
//    //    {
//    //        const Int32             w             = Max(1, width >> (i + 1));
//    //        const Int32             h             = Max(1, height >> (i + 1));
//    //        const Viewport          vpDown        = { static_cast<UInt16>(w), static_cast<UInt16>(h) };
//    //        const StringView        name          = FormatToTmpBuffer("Bloom Down Texture %d", i);
//    //        const TextureHandle     bloomDownTexh = CreateTexture2D(w, h, eTextureFormat::RG11B10_Float, eAttachment::RenderTarget, name);
//    //        const StringView        fbhName       = FormatToTmpBuffer("Bloom Down FrameBuffer %d", i);
//    //        const FrameBufferHandle bloomDownFbh  = CreateFrameBuffer(bloomDownTexh, true, fbhName);
//
//    //        RenderPass pass = {};
//    //        pass.SetProgram(bloomDownProgram);
//    //        pass.AddTexture(inputTexh, eShader::PixelShader, t_input0Slot);
//    //        pass.AddSampler(linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//    //        pass.AddConstantBuffer(screenCbh, eShader::PixelShader, CB_SCREEN_SLOT);
//    //        pass.SetFrameBuffer(bloomDownFbh, true);
//    //        pass.SetViewport(vpDown);
//    //        pass.SetRenderState(RenderState::kDefault);
//    //        pass.SetBlendState(BlendState::kOpaque, 0);
//    //        pass.SetStencil(StencilState::kDisable, StencilState::kDisable, 0);
//    //        // pass.SetExecutor(EmplacedDelegate<&BloomFullScreenTriExecutor::Submit, BloomFullScreenTriExecutor>(w, h));
//    //        // todo
//
//    //        graph.AddPass(std::move(pass));
//    //        inputTexh = bloomDownTexh;
//    //    }
//
//    //    // up
//    //    for (Int32 i = numBlurPasses - 2; i >= 0; --i)
//    //    {
//    //        const Int32             w           = Max(1, width >> (i + 1));
//    //        const Int32             h           = Max(1, height >> (i + 1));
//    //        const Viewport          vpUp        = { static_cast<UInt16>(w), static_cast<UInt16>(h) };
//    //        const StringView        name        = FormatToTmpBuffer("Bloom Up Texture %d", i);
//    //        const TextureHandle     bloomUpTexh = CreateTexture2D(w, h, eTextureFormat::RG11B10_Float, eAttachment::RenderTarget, name);
//    //        const StringView        fbhName     = FormatToTmpBuffer("Bloom Up FrameBuffer %d", i);
//    //        const FrameBufferHandle bloomUpFbh  = CreateFrameBuffer(bloomUpTexh, true, fbhName);
//
//    //        RenderPass pass;
//    //        pass.SetProgram(bloomUpProgram);
//    //        pass.AddTexture(inputTexh, eShader::PixelShader, t_input0Slot);
//    //        pass.AddSampler(linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//    //        pass.AddConstantBuffer(screenCbh, eShader::PixelShader, CB_SCREEN_SLOT);
//    //        pass.SetFrameBuffer(bloomUpFbh, true);
//    //        pass.SetViewport(vpUp);
//    //        pass.SetRenderState(RenderState::kDefault);
//    //        pass.SetBlendState(BlendState::kOpaque, 0);
//    //        pass.SetStencil(StencilState::kDisable, StencilState::kDisable, 0);
//    //        //            pass.SetExecutor(BloomFullScreenTriExecutor { w, h });
//
//    //        graph.AddPass(std::move(pass));
//    //        inputTexh = bloomUpTexh;
//    //    }
//
//    //    // combine
//    //    const TextureHandle     texh = CreateTexture2D(width, height, eTextureFormat::RG11B10_Float, eAttachment::RenderTarget, "Bloom Combine Texture");
//    //    const FrameBufferHandle fbh  = CreateFrameBuffer(texh, true, "Bloom Combine FrameBuffer");
//
//    //    RenderPass pass;
//    //    pass.SetProgram(bloomCombineProgram)
//    //        .AddTexture(originalTexh, eShader::PixelShader, t_input0Slot)
//    //        .AddTexture(inputTexh, eShader::PixelShader, t_input1Slot)
//    //        .AddSampler(linearClampSampler, eShader::PixelShader, s_linearClampSlot)
//    //        .AddConstantBuffer(environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
//    //        .SetFrameBuffer(fbh, true)
//    //        .SetViewport(vp)
//    //        .SetRenderState(RenderState::kDefault)
//    //        .SetBlendState(BlendState::kOpaque, 0)
//    //        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
//    //        .SetExecutor(BloomFullScreenTriExecutor { width, height });   // restore full screen size
//
//    //    graph.AddPass(std::move(pass));
//    //    inputTexh = texh;
//    //}
//
//    //// tone mapping
//    // if (toneMapping.bEnabled)
//    //{
//    //     const TextureHandle     texh = CreateTexture2D(width, height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget, "Tone Mapping Texture");
//    //     const FrameBufferHandle fbh  = CreateFrameBuffer(texh, true, "Tone Mapping FrameBuffer");
//
//    //    RenderPass pass;
//    //    pass.SetProgram(toneMappingProgram)
//    //        .AddTexture(inputTexh, eShader::PixelShader, t_input0Slot)
//    //        .AddSampler(linearWrapSampler, eShader::PixelShader, s_linearWrapSlot)
//    //        .AddConstantBuffer(environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
//    //        .SetFrameBuffer(fbh, true)
//    //        .SetViewport(vp)
//    //        .SetRenderState(RenderState::kDefault)
//    //        .SetBlendState(BlendState::kOpaque, 0)
//    //        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
//    //        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
//
//    //    graph.AddPass(std::move(pass));
//    //    inputTexh = texh;
//    //}
//
//    //// fxaa
//    // if (fxaa.bEnabled)
//    //{
//    //     const TextureHandle     texh = CreateTexture2D(width, height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget, "FXAA Texture");
//    //     const FrameBufferHandle fbh  = CreateFrameBuffer(texh, true, "FXAA FrameBuffer");
//
//    //    RenderPass pass;
//    //    pass.AddTexture(inputTexh, eShader::PixelShader, t_input0Slot)
//    //        .AddSampler(linearWrapSampler, eShader::PixelShader, s_linearWrapSlot)
//    //        .AddSampler(anisotropicSampler, eShader::PixelShader, s_anisotropic4WrapSlot)
//    //        .AddConstantBuffer(environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
//    //        .AddConstantBuffer(screenCbh, eShader::PixelShader, CB_SCREEN_SLOT)
//    //        .SetFrameBuffer(fbh, true)
//    //        .SetViewport(vp)
//    //        .SetRenderState(RenderState::kDefault)
//    //        .SetBlendState(BlendState::kOpaque, 0)
//    //        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
//    //        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
//
//    //    switch (fxaa.quality)
//    //    {
//    //        case eFxaaQuality::VeryLow:
//    //            pass.SetProgram(fxaaVeryLowProgram);
//    //            break;
//
//    //        case eFxaaQuality::Low:
//    //            pass.SetProgram(fxaaLowProgram);
//    //            break;
//
//    //        case eFxaaQuality::Medium:
//    //            pass.SetProgram(fxaaMediumProgram);
//    //            break;
//
//    //        case eFxaaQuality::High:
//    //            pass.SetProgram(fxaaHighProgram);
//    //            break;
//
//    //        case eFxaaQuality::VeryHigh:
//    //            pass.SetProgram(fxaaVeryHighProgram);
//    //            break;
//
//    //        case eFxaaQuality::Ultra:
//    //            pass.SetProgram(fxaaUltraProgram);
//    //            break;
//
//    //        default:
//    //            pass.SetProgram(fxaaMediumProgram);
//    //            JAM_UNREACHABLE;
//    //            break;
//    //    };
//
//    //    graph.AddPass(std::move(pass));
//    //    inputTexh = texh;
//    //}
//
//    // done
//    graph.Compile();
//}
//
//bool BasicRendererContext::CheckEnvironmentDiff(
//    const Environment& _env) const
//{
//    const DirectionalLightData&   dirLight    = _env.GetDirectionalLight();
//    const ImageBasedLightingData& ibl         = _env.GetIBL();
//    const SkyboxData&             skybox      = _env.GetSkybox();
//    const SSAOData&               ssao        = _env.GetSSAO();
//    const FogData&                fog         = _env.GetFog();
//    const ToneMappingData&        toneMapping = _env.GetToneMapping();
//    const BloomData&              bloom       = _env.GetBloom();
//    const FXAAData&               fxaa        = _env.GetFXAA();
//
//    bool bDiff = false;
//
//    bDiff |= (dirLight.bEnabled != bDirLightEnabled);
//    bDiff |= (dirLight.bShadowEnabled != bDirLightShadowEnabled);
//    bDiff |= (ibl.pIrradianceTex != pIrrdianceMap);
//    bDiff |= (ibl.pSpecularTex != pSpecularMap);
//    bDiff |= (ibl.pBrdfLUTTex != pBrdfLUTMap);
//    bDiff |= (skybox.pSkyboxTeX != pSkyboxTeX);
//    bDiff |= (bloom.numBlurPasses != numBloomPasses);
//    bDiff |= (ssao.bEnabled != bSSAOEnabled);
//    bDiff |= (fog.bEnabled != bFogEnabled);
//    bDiff |= (toneMapping.bEnabled != bToneMappingEnabled);
//    bDiff |= (bloom.bEnabled != bBloomEnabled);
//    bDiff |= (fxaa.bEnabled != bFXAAEnabled);
//
//    return bDiff;
//}
//
//}   // namespace Jam
//
//namespace Jam
//{
//
//void BasicRenderer::Init()
//{
//    JAM_ASSERT(!g_basicRendererCtx, "BasicRenderer already initialized");
//
//    JAM_FUNCTION_PROFILE();
//    g_basicRendererCtx         = new BasicRendererContext;
//    BasicRendererContext* pCtx = g_basicRendererCtx;
//
//    // vertex shaders
//    {
//        pCtx->pbrVsh = gfx.CreateShader({ shader::PBRVS_Bytecode, shader::PBRVS_Size });
//        gfx.SetShaderName(pCtx->pbrVsh, "Jam engine predefined shader: pbrVsh");
//
//        pCtx->fullscreenVsh = gfx.CreateShader({ shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
//        gfx.SetShaderName(pCtx->fullscreenVsh, "Jam engine predefined shader: fullscreenVsh");
//
//        pCtx->pbrLightVolumeVsh = gfx.CreateShader({ shader::PBRLightVolumeShader_VS_Bytecode, shader::PBRLightVolumeShader_VS_Size });
//        gfx.SetShaderName(pCtx->pbrLightVolumeVsh, "Jam engine predefined shader: pbrLightVolumeVsh");
//
//        pCtx->pbrEnvironmentLIghtVolumeVsh = gfx.CreateShader({ shader::PBRLightVolumeShaderForDirectional_VS_Bytecode, shader::PBRLightVolumeShaderForDirectional_VS_Size });
//        gfx.SetShaderName(pCtx->pbrEnvironmentLIghtVolumeVsh, "Jam engine predefined shader: pbrEnvironmentLightVolumeVsh");
//
//        pCtx->csmVsh = gfx.CreateShader({ shader::CascadeShaderMappingShader_VS_Bytecode, shader::CascadeShaderMappingShader_VS_Size });
//        gfx.SetShaderName(pCtx->csmVsh, "Jam engine predefined shader: csmVsh");
//
//        pCtx->osmVsh = gfx.CreateShader({ shader::OmniDirectionalShaderMappingShader_VS_Bytecode, shader::OmniDirectionalShaderMappingShader_VS_Size });
//        gfx.SetShaderName(pCtx->osmVsh, "Jam engine predefined shader: osmVsh");
//
//        pCtx->shadowMappingVsh = gfx.CreateShader({ shader::ShadowMappingShader_VS_Bytecode, shader::ShadowMappingShader_VS_Size });
//        gfx.SetShaderName(pCtx->shadowMappingVsh, "Jam engine predefined shader: shadowMappingVsh");
//
//        pCtx->skyboxVsh = gfx.CreateShader({ shader::SkyboxShader_VS_Bytecode, shader::SkyboxShader_VS_Size });
//        gfx.SetShaderName(pCtx->skyboxVsh, "Jam engine predefined shader: skyboxVsh");
//    }
//
//    // geometry shaders
//    {
//        pCtx->csmLevel3Gsh = gfx.CreateShader({ shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Size });
//        gfx.SetShaderName(pCtx->csmLevel3Gsh, "Jam engine predefined shader: csmLevel3Gsh");
//
//        pCtx->osmGsh = gfx.CreateShader({ shader::OmniDirectionalShaderMappingShader_GS_Bytecode, shader::OmniDirectionalShaderMappingShader_GS_Size });
//        gfx.SetShaderName(pCtx->osmGsh, "Jam engine predefined shader: osmGsh");
//    }
//
//    // pixel shaders
//    {
//        pCtx->pbrGBufferPsh = gfx.CreateShader({ shader::PBRGBufferShader_PS_Bytecode, shader::PBRGBufferShader_PS_Size });
//        gfx.SetShaderName(pCtx->pbrGBufferPsh, "Jam engine predefined shader: pbrGBufferPsh");
//
//        pCtx->pbrLightingPsh = gfx.CreateShader({ shader::PBRLightingShader_PS_Bytecode, shader::PBRLightingShader_PS_Size });
//        gfx.SetShaderName(pCtx->pbrLightingPsh, "Jam engine predefined shader: pbrLightingPsh");
//
//        pCtx->pbrLightVolumePsh = gfx.CreateShader({ shader::PBRLightVolumeShader_PS_Bytecode, shader::PBRLightVolumeShader_PS_Size });
//        gfx.SetShaderName(pCtx->pbrLightVolumePsh, "Jam engine predefined shader: pbrLightVolumePsh");
//
//        pCtx->pbrEnvironmentLightVolumePsh = gfx.CreateShader({ shader::PBRLightVolumeShaderForDirectional_PS_Bytecode, shader::PBRLightVolumeShaderForDirectional_PS_Size });
//        gfx.SetShaderName(pCtx->pbrEnvironmentLightVolumePsh, "Jam engine predefined shader: pbrEnvironmentLightVolumePsh");
//
//        pCtx->csmPsh = gfx.CreateShader({ shader::CascadeShaderMappingShader_PS_Bytecode, shader::CascadeShaderMappingShader_PS_Size });
//        gfx.SetShaderName(pCtx->csmPsh, "Jam engine predefined shader: csmPsh");
//
//        pCtx->osmPsh = gfx.CreateShader({ shader::OmniDirectionalShaderMappingShader_PS_Bytecode, shader::OmniDirectionalShaderMappingShader_PS_Size });
//        gfx.SetShaderName(pCtx->osmPsh, "Jam engine predefined shader: osmPsh");
//
//        pCtx->shadowMappingPsh = gfx.CreateShader({ shader::ShadowMappingShader_PS_Bytecode, shader::ShadowMappingShader_PS_Size });
//        gfx.SetShaderName(pCtx->shadowMappingPsh, "Jam engine predefined shader: shadowMappingPsh");
//
//        pCtx->skyboxPsh = gfx.CreateShader({ shader::SkyboxShader_PS_Bytecode, shader::SkyboxShader_PS_Size });
//        gfx.SetShaderName(pCtx->skyboxPsh, "Jam engine predefined shader: skyboxPsh");
//
//        pCtx->ssaoPsh = gfx.CreateShader({ shader::SSAOPS_Bytecode, shader::SSAOPS_Size });
//        gfx.SetShaderName(pCtx->ssaoPsh, "Jam engine predefined shader: ssaoPsh");
//
//        pCtx->ssaoBlurXPsh = gfx.CreateShader({ shader::SSAOBlurXPS_Bytecode, shader::SSAOBlurXPS_Size });
//        gfx.SetShaderName(pCtx->ssaoBlurXPsh, "Jam engine predefined shader: ssaoBlurXPsh");
//
//        pCtx->ssaoBlurYPsh = gfx.CreateShader({ shader::SSAOBlurYPS_Bytecode, shader::SSAOBlurYPS_Size });
//        gfx.SetShaderName(pCtx->ssaoBlurYPsh, "Jam engine predefined shader: ssaoBlurYPsh");
//
//        pCtx->bloomUpPsh = gfx.CreateShader({ shader::BloomBlurUpPS_Bytecode, shader::BloomBlurUpPS_Size });
//        gfx.SetShaderName(pCtx->bloomUpPsh, "Jam engine predefined shader: bloomXPsh");
//
//        pCtx->bloomDownPsh = gfx.CreateShader({ shader::BloomBlurDownPS_Bytecode, shader::BloomBlurDownPS_Size });
//        gfx.SetShaderName(pCtx->bloomDownPsh, "Jam engine predefined shader: bloomYPsh");
//
//        pCtx->bloomCombinePsh = gfx.CreateShader({ shader::BloomCombinePS_Bytecode, shader::BloomCombinePS_Size });
//        gfx.SetShaderName(pCtx->bloomCombinePsh, "Jam engine predefined shader: bloomCombinePsh");
//
//        pCtx->toneMappingPsh = gfx.CreateShader({ shader::ToneMappingPS_Bytecode, shader::ToneMappingPS_Size });
//        gfx.SetShaderName(pCtx->toneMappingPsh, "Jam engine predefined shader: toneMappingPsh");
//
//        pCtx->fxaaVeryLowPsh = gfx.CreateShader({ shader::FXAA_VeryLow_PS_Bytecode, shader::FXAA_VeryLow_PS_Size });
//        gfx.SetShaderName(pCtx->fxaaVeryLowPsh, "Jam engine predefined shader: fxaaVeryLowPsh");
//
//        pCtx->fxaaLowPsh = gfx.CreateShader({ shader::FXAA_VeryLow_PS_Bytecode, shader::FXAA_VeryLow_PS_Size });
//        gfx.SetShaderName(pCtx->fxaaLowPsh, "Jam engine predefined shader: fxaaLowPsh");
//
//        pCtx->fxaaMediumPsh = gfx.CreateShader({ shader::FXAA_Medium_PS_Bytecode, shader::FXAA_Medium_PS_Size });
//        gfx.SetShaderName(pCtx->fxaaMediumPsh, "Jam engine predefined shader: fxaaMediumPsh");
//
//        pCtx->fxaaHighPsh = gfx.CreateShader({ shader::FXAA_High_PS_Bytecode, shader::FXAA_High_PS_Size });
//        gfx.SetShaderName(pCtx->fxaaHighPsh, "Jam engine predefined shader: fxaaHighPsh");
//
//        pCtx->fxaaVeryHighPsh = gfx.CreateShader({ shader::FXAA_VeryHigh_PS_Bytecode, shader::FXAA_VeryHigh_PS_Size });
//        gfx.SetShaderName(pCtx->fxaaVeryHighPsh, "Jam engine predefined shader: fxaaVeryHighPsh");
//
//        pCtx->fxaaUltraPsh = gfx.CreateShader({ shader::FXAA_Ultra_PS_Bytecode, shader::FXAA_Ultra_PS_Size });
//        gfx.SetShaderName(pCtx->fxaaUltraPsh, "Jam engine predefined shader: fxaaUltraPsh");
//
//        pCtx->fogPsh = gfx.CreateShader({ shader::FogPS_Bytecode, shader::FogPS_Size });
//        gfx.SetShaderName(pCtx->fogPsh, "Jam engine predefined shader: fogPsh");
//
//        pCtx->passTexhroughPsh = gfx.CreateShader({ shader::PassThroughPS_Bytecode, shader::PassThroughPS_Size });
//        gfx.SetShaderName(pCtx->passTexhroughPsh, "Jam engine predefined shader: passTexhroughPsh");
//    }
//
//    // programs
//    {
//        pCtx->pbrGBufferProgram                = gfx.CreateProgram(pCtx->pbrVsh, pCtx->pbrGBufferPsh);
//        pCtx->pbrLightingProgram               = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->pbrLightingPsh);
//        pCtx->pbrLightVolumeProgram            = gfx.CreateProgram(pCtx->pbrLightVolumeVsh, pCtx->pbrLightVolumePsh);
//        pCtx->pbrEnvironmentLightVolumeProgram = gfx.CreateProgram(pCtx->pbrEnvironmentLIghtVolumeVsh, pCtx->pbrEnvironmentLightVolumePsh);
//        pCtx->csmLevel3CasterProgram           = gfx.CreateProgram(pCtx->csmVsh, pCtx->csmPsh, pCtx->csmLevel3Gsh, kInvalidHandle, kInvalidHandle);
//        pCtx->osmCasterProgram                 = gfx.CreateProgram(pCtx->osmVsh, pCtx->osmPsh, pCtx->osmGsh, kInvalidHandle, kInvalidHandle);
//        pCtx->shadowCasterProgram              = gfx.CreateProgram(pCtx->shadowMappingVsh, pCtx->shadowMappingPsh);
//        pCtx->skyboxProgram                    = gfx.CreateProgram(pCtx->skyboxVsh, pCtx->skyboxPsh);
//        pCtx->ssaoProgram                      = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->ssaoPsh);
//        pCtx->ssaoBlurXProgram                 = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->ssaoBlurXPsh);
//        pCtx->ssaoBlurYProgram                 = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->ssaoBlurYPsh);
//        pCtx->bloomUpProgram                   = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->bloomUpPsh);
//        pCtx->bloomDownProgram                 = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->bloomDownPsh);
//        pCtx->bloomCombineProgram              = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->bloomCombinePsh);
//        pCtx->toneMappingProgram               = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->toneMappingPsh);
//        pCtx->fxaaVeryLowProgram               = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->fxaaVeryLowPsh);
//        pCtx->fxaaLowProgram                   = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->fxaaLowPsh);
//        pCtx->fxaaMediumProgram                = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->fxaaMediumPsh);
//        pCtx->fxaaHighProgram                  = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->fxaaHighPsh);
//        pCtx->fxaaVeryHighProgram              = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->fxaaVeryHighPsh);
//        pCtx->fxaaUltraProgram                 = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->fxaaUltraPsh);
//        pCtx->fogProgram                       = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->fogPsh);
//        pCtx->passTexhroughProgram             = gfx.CreateProgram(pCtx->fullscreenVsh, pCtx->passTexhroughPsh);
//    }
//
//    // constant buffers
//    {
//        pCtx->transformCbh = gfx.CreateConstantBuffer(sizeof(CB_TRANSFORM));
//        gfx.SetConstantBufferName(pCtx->transformCbh, "Jam engine predefined constant buffer: CB_TRANSFORM");
//
//        pCtx->cameraCbh = gfx.CreateConstantBuffer(sizeof(CB_CAMERA));
//        gfx.SetConstantBufferName(pCtx->cameraCbh, "Jam engine predefined constant buffer: CB_CAMERA");
//
//        pCtx->materialCbh = gfx.CreateConstantBuffer(sizeof(CB_MATERIAL));
//        gfx.SetConstantBufferName(pCtx->materialCbh, "Jam engine predefined constant buffer: CB_MATERIAL");
//
//        pCtx->lightCbh = gfx.CreateConstantBuffer(sizeof(CB_LIGHT));
//        gfx.SetConstantBufferName(pCtx->lightCbh, "Jam engine predefined constant buffer: CB_LIGHT");
//
//        pCtx->environmentCbh = gfx.CreateConstantBuffer(sizeof(CB_ENVIRONMENT));
//        gfx.SetConstantBufferName(pCtx->environmentCbh, "Jam engine predefined constant buffer: CB_ENVIRONMENT");
//
//        pCtx->shadowCasterCbh = gfx.CreateConstantBuffer(sizeof(CB_SHADOW_CASTER));
//        gfx.SetConstantBufferName(pCtx->shadowCasterCbh, "Jam engine predefined constant buffer: CB_SHADOW_CASTER");
//
//        pCtx->csmCasterCbh = gfx.CreateConstantBuffer(sizeof(CB_CASCADE_SHADOW_CASTER));
//        gfx.SetConstantBufferName(pCtx->csmCasterCbh, "Jam engine predefined constant buffer: CB_CASCADE_SHADOW_CASTER");
//
//        pCtx->osmCasterCbh = gfx.CreateConstantBuffer(sizeof(CB_OMNIDIRECTIONAL_SHADOW_CASTER));
//        gfx.SetConstantBufferName(pCtx->osmCasterCbh, "Jam engine predefined constant buffer: CB_OMNIDIRECTIONAL_SHADOW_CASTER");
//
//        pCtx->ssaoCbh = gfx.CreateConstantBuffer(sizeof(CB_SSAO_KERNEL));
//        gfx.SetConstantBufferName(pCtx->ssaoCbh, "Jam engine predefined constant buffer: CB_SSAO_KERNEL");
//
//        pCtx->screenCbh = gfx.CreateConstantBuffer(sizeof(CB_SCREEN));
//        gfx.SetConstantBufferName(pCtx->screenCbh, "Jam engine predefined constant buffer: CB_SCREEN");
//    }
//
//    // full screen tri
//    {
//        constexpr VertexPosTex2D kVerts[] = {
//            { Vec2(-1.f, -1.f), Vec2(0.f,  1.f) },
//            { Vec2(-1.f,  3.f), Vec2(0.f, -1.f) },
//            {  Vec2(3.f, -1.f), Vec2(2.f,  1.f) }
//        };
//        constexpr UInt16 kIndices[] = { 0, 1, 2 };
//        pCtx->fullScreenTri.Load(kVerts, VertexPosTex2D::s_layout, { kIndices });
//    }
//
//    // light volume full tri
//    {
//        constexpr VertexPos3D kVerts[] = {
//            { Vec3(-1.f, -1.f, 0.f) },
//            { Vec3(-1.f, 3.f, 0.f) },
//            { Vec3(3.f, -1.f, 0.f) }
//        };
//        constexpr UInt16 kIndices[] = { 0, 1, 2 };
//        pCtx->lightVolumeFullScreenTri.Load(kVerts, VertexPos3D::s_layout, { kIndices });
//    }
//
//    // skybox cube
//    {
//        constexpr Vec3 whd = { 1.f, 1.f, 1.f };
//        pCtx->skyboxCube   = CreateCubeMesh(whd, VertexPos3D::s_layout);
//    }
//
//    // unit sphere
//    {
//        pCtx->lightVolumeUnitSphere = CreateSphereMesh(1.f, 2, VertexPos3D::s_layout);
//    }
//
//    // unit corn
//    {
//        pCtx->lightVolumeUnitCone = CreateCone(1.f, 1.f, 20, VertexPos3D::s_layout);
//    }
//
//    // ssao
//    {
//        pCtx->BakeSSAOKernel();
//        pCtx->BakeSSAONoise();
//    }
//
//    // sampler
//    {
//        pCtx->linearWrapSampler.filter   = eTextureFilter::MinLinear_MagLinear_MipLinear;
//        pCtx->linearWrapSampler.addressU = eTextureAddressMode::Wrap;
//        pCtx->linearWrapSampler.addressV = eTextureAddressMode::Wrap;
//        pCtx->linearWrapSampler.addressW = eTextureAddressMode::Wrap;
//
//        pCtx->linearClampSampler.filter   = eTextureFilter::MinLinear_MagLinear_MipLinear;
//        pCtx->linearClampSampler.addressU = eTextureAddressMode::Clamp;
//        pCtx->linearClampSampler.addressV = eTextureAddressMode::Clamp;
//        pCtx->linearClampSampler.addressW = eTextureAddressMode::Clamp;
//
//        pCtx->pointClampSampler.filter   = eTextureFilter::MinPoint_MagPoint_MipPoint;
//        pCtx->pointClampSampler.addressU = eTextureAddressMode::Clamp;
//        pCtx->pointClampSampler.addressV = eTextureAddressMode::Clamp;
//        pCtx->pointClampSampler.addressW = eTextureAddressMode::Clamp;
//
//        pCtx->shadowLinearClampSampler.filter      = eTextureFilter::MinLinear_MagLinear_MipLinear;
//        pCtx->shadowLinearClampSampler.addressU    = eTextureAddressMode::Clamp;
//        pCtx->shadowLinearClampSampler.addressV    = eTextureAddressMode::Clamp;
//        pCtx->shadowLinearClampSampler.addressW    = eTextureAddressMode::Clamp;
//        pCtx->shadowLinearClampSampler.compareFunc = eCompareFunc::LessEqual;
//
//        pCtx->anisotropicSampler.filter   = eTextureFilter::Anisotropic4;
//        pCtx->anisotropicSampler.addressU = eTextureAddressMode::Wrap;
//        pCtx->anisotropicSampler.addressV = eTextureAddressMode::Wrap;
//        pCtx->anisotropicSampler.addressW = eTextureAddressMode::Wrap;
//
//        // 후면 컬링, 깊이 테스트 활성화, 깊이 쓰기 활성화
//        pCtx->depthTestState = RenderState::kDepthTest;
//
//        // 전면 컬링, 깊이 비활성화
//        pCtx->lightVolumeState = RenderState { ePrimitive::TriangleList, eCullMode::Front, false, false, false, false, false, false, false, false, eCompareFunc::LessEqual, false };
//
//        // 전면 컬링, 깊이 테스트 활성화, 깊이 쓰기 비활성화
//        pCtx->skyboxState = RenderState { ePrimitive::TriangleList, eCullMode::Front, false, false, false, false, false, false, false, false, eCompareFunc::LessEqual, false };
//    }
//}
//
//void BasicRenderer::Shutdown()
//{
//    JAM_ASSERT(g_basicRendererCtx, "BasicRenderer not initialized");
//    BasicRendererContext* pCtx = g_basicRendererCtx;
//
//    JAM_DESTROY_SHADER(pCtx->pbrVsh);
//    JAM_DESTROY_SHADER(pCtx->fullscreenVsh);
//    JAM_DESTROY_SHADER(pCtx->pbrLightVolumeVsh);
//    JAM_DESTROY_SHADER(pCtx->pbrEnvironmentLIghtVolumeVsh);
//    JAM_DESTROY_SHADER(pCtx->csmVsh);
//    JAM_DESTROY_SHADER(pCtx->osmVsh);
//    JAM_DESTROY_SHADER(pCtx->shadowMappingVsh);
//    JAM_DESTROY_SHADER(pCtx->skyboxVsh);
//
//    JAM_DESTROY_SHADER(pCtx->csmLevel3Gsh);
//    JAM_DESTROY_SHADER(pCtx->osmGsh);
//
//    JAM_DESTROY_SHADER(pCtx->pbrGBufferPsh);
//    JAM_DESTROY_SHADER(pCtx->pbrLightingPsh);
//    JAM_DESTROY_SHADER(pCtx->pbrLightVolumePsh);
//    JAM_DESTROY_SHADER(pCtx->pbrEnvironmentLightVolumePsh);
//    JAM_DESTROY_SHADER(pCtx->csmPsh);
//    JAM_DESTROY_SHADER(pCtx->osmPsh);
//    JAM_DESTROY_SHADER(pCtx->shadowMappingPsh);
//    JAM_DESTROY_SHADER(pCtx->skyboxPsh);
//    JAM_DESTROY_SHADER(pCtx->ssaoPsh);
//    JAM_DESTROY_SHADER(pCtx->ssaoBlurXPsh);
//    JAM_DESTROY_SHADER(pCtx->ssaoBlurYPsh);
//    JAM_DESTROY_SHADER(pCtx->bloomUpPsh);
//    JAM_DESTROY_SHADER(pCtx->bloomDownPsh);
//    JAM_DESTROY_SHADER(pCtx->bloomCombinePsh);
//    JAM_DESTROY_SHADER(pCtx->toneMappingPsh);
//    JAM_DESTROY_SHADER(pCtx->fxaaVeryLowPsh);
//    JAM_DESTROY_SHADER(pCtx->fxaaLowPsh);
//    JAM_DESTROY_SHADER(pCtx->fxaaMediumPsh);
//    JAM_DESTROY_SHADER(pCtx->fxaaHighPsh);
//    JAM_DESTROY_SHADER(pCtx->fxaaVeryHighPsh);
//    JAM_DESTROY_SHADER(pCtx->fxaaUltraPsh);
//    JAM_DESTROY_SHADER(pCtx->fogPsh);
//    JAM_DESTROY_SHADER(pCtx->passTexhroughPsh);
//
//    JAM_DESTROY_PROGRAM(pCtx->pbrGBufferProgram);
//    JAM_DESTROY_PROGRAM(pCtx->pbrLightingProgram);
//    JAM_DESTROY_PROGRAM(pCtx->pbrLightVolumeProgram);
//    JAM_DESTROY_PROGRAM(pCtx->pbrEnvironmentLightVolumeProgram);
//    JAM_DESTROY_PROGRAM(pCtx->csmLevel3CasterProgram);
//    JAM_DESTROY_PROGRAM(pCtx->osmCasterProgram);
//    JAM_DESTROY_PROGRAM(pCtx->shadowCasterProgram);
//    JAM_DESTROY_PROGRAM(pCtx->skyboxProgram);
//    JAM_DESTROY_PROGRAM(pCtx->ssaoProgram);
//    JAM_DESTROY_PROGRAM(pCtx->ssaoBlurXProgram);
//    JAM_DESTROY_PROGRAM(pCtx->ssaoBlurYProgram);
//
//    JAM_DESTROY_PROGRAM(pCtx->bloomUpProgram);
//    JAM_DESTROY_PROGRAM(pCtx->bloomDownProgram);
//    JAM_DESTROY_PROGRAM(pCtx->bloomCombineProgram);
//    JAM_DESTROY_PROGRAM(pCtx->toneMappingProgram);
//    JAM_DESTROY_PROGRAM(pCtx->fxaaVeryLowProgram);
//    JAM_DESTROY_PROGRAM(pCtx->fxaaLowProgram);
//    JAM_DESTROY_PROGRAM(pCtx->fxaaMediumProgram);
//    JAM_DESTROY_PROGRAM(pCtx->fxaaHighProgram);
//    JAM_DESTROY_PROGRAM(pCtx->fxaaVeryHighProgram);
//    JAM_DESTROY_PROGRAM(pCtx->fxaaUltraProgram);
//    JAM_DESTROY_PROGRAM(pCtx->fogProgram);
//    JAM_DESTROY_PROGRAM(pCtx->passTexhroughProgram);
//
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->transformCbh);
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->cameraCbh);
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->materialCbh);
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->lightCbh);
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->environmentCbh);
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->shadowCasterCbh);
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->csmCasterCbh);
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->osmCasterCbh);
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->ssaoCbh);
//    JAM_DESTROY_CONSTANT_BUFFER(pCtx->screenCbh);
//
//    JAM_DESTROY_TEXTURE(pCtx->ssaoNoiseTexture);
//
//    JAM_DESTROY_TEXTURE(pCtx->hdrTexh);
//    JAM_DESTROY_TEXTURE(pCtx->depthTexh);
//    JAM_DESTROY_TEXTURE(pCtx->gbufferAlbedoTexh);
//    JAM_DESTROY_TEXTURE(pCtx->gbufferMaterialTexh);
//    JAM_DESTROY_TEXTURE(pCtx->gbufferNormalTexh);
//    JAM_DESTROY_TEXTURE(pCtx->gbufferLightingTexh);
//
//    JAM_DESTROY_FRAME_BUFFER(pCtx->hdrNoDepthFbh);
//    JAM_DESTROY_FRAME_BUFFER(pCtx->hdrDepthFbh);
//
//    JAM_DELETE(pCtx);
//}
//
//void BasicRenderer::Render(
//    const RenderView& _renderView,
//    const Ref<Scene>& _scene)
//{
//    BasicRendererContext* pCtx = g_basicRendererCtx;
//
//    // basic renderer는 EnvironmentComponent를 필요로함. 없으면 생성
//    Entity root = _scene->GetRootEntity();
//    JAM_ASSERT(root, "Scene has no root entity");
//    if (!root.HasComponent<EnvironmentComponent>())
//    {
//        root.CreateComponent<EnvironmentComponent>();
//    }
//
//    EnvironmentComponent& ec     = root.GetComponent<EnvironmentComponent>();
//    Environment&          env    = ec.environment;
//    const Viewport        vp     = _renderView.vp;
//    const Int32           width  = vp.w;
//    const Int32           height = vp.h;
//
//    // check need to rebuild
//    bool bNeedRebuild = false;
//
//    // 기존 env와 다른 인스턴스라면 리빌드 필요
//    if (pCtx->pEnv != &env)
//    {
//        bNeedRebuild |= true;
//    }
//
//    // 사이즈가 달라졌다면, 리빌드 필요
//    if (width != pCtx->width || height != pCtx->height)
//    {
//        pCtx->UploadScreen(width, height);
//        bNeedRebuild |= true;
//    }
//
//    // 환경 설정이 변경되었다면, 리빌드 필요
//    if (env.IsDirty())
//    {
//        bNeedRebuild |= pCtx->CheckEnvironmentDiff(env);
//        pCtx->UploadEnvironment(env);
//        env.ClearDirty();
//    }
//
//    // 리빌드 필요시
//    if (bNeedRebuild)
//    {
//        // todo: scene을 넘기는 것이 아니라. renderable object들만 넘기도록 수정 필요
//        pCtx->BuildRenderGraph(width, height, env, _scene);
//    }
//
//    // 카메라 행렬 갱신
//    pCtx->UploadCamera(_renderView.eye, _renderView.view, _renderView.proj);
//
//    // 렌더 그래프 실행
//    pCtx->renderGraph.Execute(_renderView.fbh, _renderView.vp);
//}
//
//TextureHandle BasicRenderer::GetDepthTexture()
//{
//    return g_basicRendererCtx->depthTexh;
//}
//
//// void BasicRenderer::Render(const RenderView& _view)
////{
////    // camera buffer
////    g_basicRendererCtx->UploadCamera(_view.eyePos, _view.view, _view.proj);
////
////    // render
////    g_basicRendererCtx->renderGraph.Execute(_view.fbh, _view.vp);
////}
////
//// void BasicRenderer::UploadEnvironmentData(const Environment& _env)
////{
////    g_basicRendererCtx->UploadEnvironment(_env);
////}
////
//// void BasicRenderer::UploadCascadeShadowCasterData(const Frustum& _viewFrustumInW, const Environment& _env)
////{
////    const DirectionalLightData& dirLight = _env.GetDirectionalLight();
////
////    const Int32 cascadeLevel = dirLight.cascadeLevel == eCascadeLevel::level2 ? 2
////                             : dirLight.cascadeLevel == eCascadeLevel::level3 ? 3
////                             : dirLight.cascadeLevel == eCascadeLevel::level4 ? 4
////                                                                              : 1;
////
////    CB_CASCADE_SHADOW_CASTER cb       = {};
////    const float              nearZ    = _viewFrustumInW.Near();
////    const float              farZ     = _viewFrustumInW.Far();
////    const float              invRange = 1.f / Max(farZ - nearZ, 1e-6f);
////
////    float s0 = 0.f;
////    for (Int32 i = 0; i < cascadeLevel; ++i)
////    {
////        // create frustum for each cascade
////        const float   raw        = dirLight.cascadeDistances[i] * invRange;
////        const float   s1         = Clamp(raw, s0, 1.f);
////        const Frustum frustumInW = Frustum::SubFrustum(_viewFrustumInW, s0, s1);
////        s0                       = s1;
////
////        // corners & center
////        const GetCorners corners = frustumInW.GetCorners();
////        Vec3          center  = Vec3::kZero;
////        for (Int32 j = 0; j < 8; ++j)
////        {
////            center += corners.points[j];
////        }
////        center *= 1.f / 8.f;
////
////        // light view
////        const Matrix lightV = CreateViewLookTo(center, dirLight.direction);
////
////        // aabb in light space
////        AABB lightAabb = {};
////        for (Int32 j = 0; j < 8; ++j)
////        {
////            const Vec3 p = Vec3::Transform(corners.points[j], lightV);
////            lightAabb.Merge(p);
////        }
////
////        // depth ext
////        {
////            const float depth = Max(lightAabb.DepthStencil(), 1e-4f);
////            const float zPad  = depth * 0.25f + 50.0f;
////            lightAabb.min.z -= zPad;
////            lightAabb.max.z += zPad;
////        }
////
////        // create ortho
////        {
////            const Matrix lightP    = CreateOrthographic(lightAabb.Width(), lightAabb.Height(), lightAabb.min.z, lightAabb.max.z);
////            cb.cb_csmViewProj[i] = (lightV * lightP).Transpose();
////        }
////    }
////
////    gfx.UploadBuffer_(g_basicRendererCtx->csmCasterCbh, cb);
////}
////
//// void BasicRenderer::UploadOmniDirectionalShadowCasterData(const Vec3 _pos, const float _range)
////{
////    Matrix lightP = CreatePerspectiveFov(ToRad(90.f), 1.f, 0.01f, _range);
////
////    CB_OMNIDIRECTIONAL_SHADOW_CASTER cb;
////    cb.cb_odsViewProj[EnumToInt(eCubeFace::Right)]  = (CreateViewLookTo(_pos, Vec3::right) * lightP).Transpose();
////    cb.cb_odsViewProj[EnumToInt(eCubeFace::Left)]   = (CreateViewLookTo(_pos, Vec3::left) * lightP).Transpose();
////    cb.cb_odsViewProj[EnumToInt(eCubeFace::Top)]    = (CreateViewLookTo(_pos, Vec3::kUp) * lightP).Transpose();
////    cb.cb_odsViewProj[EnumToInt(eCubeFace::Bottom)] = (CreateViewLookTo(_pos, Vec3::kDown) * lightP).Transpose();
////    cb.cb_odsViewProj[EnumToInt(eCubeFace::Front)]  = (CreateViewLookTo(_pos, Vec3::kForward) * lightP).Transpose();
////    cb.cb_odsViewProj[EnumToInt(eCubeFace::Back)]   = (CreateViewLookTo(_pos, Vec3::kBackward) * lightP).Transpose();
////    gfx.UploadBuffer_(g_basicRendererCtx->osmCasterCbh, cb);
////}
////
//// void BasicRenderer::BakeSSAONoiseTexture()
////{
////    g_basicRendererCtx->BakeSSAONoise();
////}
////
//// void BasicRenderer::BakeSSAOKernel()
////{
////    g_basicRendererCtx->BakeSSAOKernel();
////}
////
//// void BasicRenderer::Submit(const Mesh& _mesh, const Material& _material, const Transform& _transform)
////{
////    g_basicRendererCtx->UploadMaterial(_material);
////    g_basicRendererCtx->UploadTransform(_transform.GetMatrix());
////    _mesh.Submit();
////}
////
//// void BasicRenderer::Submit(const Mesh& _mesh, const Transform& _transform)
////{
////    g_basicRendererCtx->UploadTransform(_transform.GetMatrix());
////    _mesh.Submit();
////}
////
//// void BasicRenderer::SubmitPointLight(const Vec3 _radiance, const float _lightIntensity, const Vec3 _pos, const float _fallOffStart, const float _fallOffEnd)
////{
////    Matrix lightMat = Matrix::CreateScale(_fallOffEnd) * Matrix::CreateTranslation(_pos);
////
////    g_basicRendererCtx->UploadPointLight(_radiance, _lightIntensity, _pos, _fallOffStart, _fallOffEnd, false);
////    g_basicRendererCtx->UploadTransform(lightMat);
////    g_basicRendererCtx->lightVolumeUnitSphere.Submit();
////}
////
//// void BasicRenderer::SubmitPointLightWithShadow(const Vec3 _radiance, const float _lightIntensity, const Vec3 _pos, const float _fallOffStart, const float _fallOffEnd, const TextureHandle _shadowMap)
////{
////    JAM_ASSERT(!_shadowMap, "Not implemented yet");
////    gfx.SetTexture(_shadowMap, eShader::PixelShader, t_shadowSlot);
////    SubmitPointLight(_radiance, _lightIntensity, _pos, _fallOffStart, _fallOffEnd);
////}
////
//// void BasicRenderer::SubmitSpotLight(const Vec3 _radiance, const float _lightIntensity, const Vec3 _pos, const Vec3 _dir, const float _fallOffStart, const float _fallOffEnd, const float _innerConeAngle, const float _outerConeAngle)
////{
////    float h        = _fallOffEnd;
////    float t        = std::tanf(_outerConeAngle);
////    float w        = t * h;
////    Vec3  pos      = _pos + _dir * h * 0.5f;
////    Quat  q        = Quat::FromToRotation(Vec3::kUp, _dir);
////    Matrix  lightMat = Matrix::CreateScale(Vec3(w, h, w)) * Matrix::CreateFromQuaternion(q) * Matrix::CreateTranslation(pos);
////    g_basicRendererCtx->UploadTransform(lightMat);
////    g_basicRendererCtx->UploadSpotLight(_radiance, _lightIntensity, _pos, _dir, _fallOffStart, _fallOffEnd, _innerConeAngle, _outerConeAngle, false);
////    g_basicRendererCtx->lightVolumeUnitCone.Submit();
////}
////
//// void BasicRenderer::SubmitSpotLightWithShadow(const Vec3 _radiance, const float _lightIntensity, const Vec3 _pos, const Vec3 _dir, const float _fallOffStart, const float _fallOffEnd, const float _innerConeAngle, const float _outerConeAngle, const TextureHandle _shadowMap)
////{
////    JAM_ASSERT(!_shadowMap, "Not implemented yet");
////    gfx.SetTexture(_shadowMap, eShader::PixelShader, t_shadowSlot);
////    SubmitSpotLight(_radiance, _lightIntensity, _pos, _dir, _fallOffStart, _fallOffEnd, _innerConeAngle, _outerConeAngle);
////}
//
////}
////
//// RenderPass BasicRenderer::CreatePBRLightPass(const PBRLightPassDesc& _desc)
////{
////    const GBufferDesc& gBufferDesc = _desc.gBufferDesc;
////
////    JAM_ASSERT(_desc.irradianceMap, "Irradiance map is required");
////    JAM_ASSERT(_desc.specularMap, "Specular map is required");
////    JAM_ASSERT(_desc.brdfLUT, "BRDF LUT is required");
////
////    RenderPass pass;
////    pass.SetProgram(g_basicRendererCtx->pbrLightingProgram)
////        .AddSampler(g_basicRendererCtx->pointClampSampler, eShader::PixelShader, s_pointClampSlot)
////        .AddSampler(g_basicRendererCtx->linearWrapSampler, eShader::PixelShader, s_linearWrapSlot)
////        .AddSampler(g_basicRendererCtx->linearClampSampler, eShader::PixelShader, s_linearClampSlot)
////        .AddConstantBuffer(g_basicRendererCtx->cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
////        .AddTexture(_desc.irradianceMap, eShader::PixelShader, t_irradianceSlot)
////        .AddTexture(_desc.specularMap, eShader::PixelShader, t_specularSlot)
////        .AddTexture(_desc.brdfLUT, eShader::PixelShader, t_brdfSlot)
////        .AddTexture(gBufferDesc.normalRoughnessTexture, eShader::PixelShader, t_gBufferNormalRoughnessSlot)
////        .AddTexture(gBufferDesc.albedoAoTexture, eShader::PixelShader, t_gBufferAlbedoAOSlot)
////        .AddTexture(gBufferDesc.emissiveMetallicTexture, eShader::PixelShader, t_gBufferEmissiveMetallicSlot)
////        .AddTexture(gBufferDesc.depthTexture, eShader::PixelShader, t_depthSlot)
////        .SetFrameBuffer(_Dedesc.frameBuffer)
////        .SetViewport(_desc.vp)
////        .SetGfxState(GfxState::Default())
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////
////    if (_desc.ssaoTexture)
////        pass.AddTexture(_desc.ssaoTexture, eShader::PixelShader, t_ssaoSlot);
////
////    return pass;
////}
////
//// RenderPass BasicRenderer::CreatePBRLightVolumePass(const PBRLightVolumePassDesc& _desc)
////{
////    const GBufferDesc& gBufferDesc = _desc.gBufferDesc;
////
////    RenderPass pass;
////    pass.SetProgram(g_basicRendererCtx->pbrLightVolumeProgram)
////        .AddSampler(g_basicRendererCtx->pointClampSampler, eShader::PixelShader, s_pointClampSlot)
////        .AddSampler(g_basicRendererCtx->linearClampSampler, eShader::PixelShader, s_linearClampSlot)
////        .AddConstantBuffer(g_basicRendererCtx->transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->cameraCbh, eShader::VertexShader, CB_CAMERA_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->lightCbh, eShader::PixelShader, CB_LIGHT_SLOT)
////        .AddTexture(gBufferDesc.normalRoughnessTexture, eShader::PixelShader, t_gBufferNormalRoughnessSlot)
////        .AddTexture(gBufferDesc.albedoAoTexture, eShader::PixelShader, t_gBufferAlbedoAOSlot)
////        .AddTexture(gBufferDesc.emissiveMetallicTexture, eShader::PixelShader, t_gBufferEmissiveMetallicSlot)
////        .AddTexture(gBufferDesc.depthTexture, eShader::PixelShader, t_depthSlot)
////        .SetFrameBuffer(_desc.frameBuffer)
////        .SetViewport(_desc.vp)
////        .SetGfxState(g_basicRendererCtx->lightVolumeState)
////        .SetBlendState(BlendState::kAdditive, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0);
////
////    return pass;
////}
////
//// RenderPass BasicRenderer::CreatePBREnvironmentLightVolumePass(const PBREnvironmentLightVolumePassDesc& _desc)
////{
////    struct Executor
////    {
////        void operator()() const
////        {
////            g_basicRendererCtx->lightVolumeFullScreenTri.Submit();
////        }
////    };
////
////    const GBufferDesc& gBufferDesc = _desc.gBufferDesc;
////
////    RenderPass pass;
////    pass.SetProgram(g_basicRendererCtx->pbrLightVolumeProgram)
////        .AddSampler(g_basicRendererCtx->pointClampSampler, eShader::PixelShader, s_pointClampSlot)
////        .AddSampler(g_basicRendererCtx->linearClampSampler, eShader::PixelShader, s_linearClampSlot)
////        .AddConstantBuffer(g_basicRendererCtx->cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
////        .AddTexture(gBufferDesc.albedoAoTexture, eShader::PixelShader, t_gBufferAlbedoAOSlot)
////        .AddTexture(gBufferDesc.normalRoughnessTexture, eShader::PixelShader, t_gBufferNormalRoughnessSlot)
////        .AddTexture(gBufferDesc.emissiveMetallicTexture, eShader::PixelShader, t_gBufferEmissiveMetallicSlot)
////        .AddTexture(gBufferDesc.depthTexture, eShader::PixelShader, t_depthSlot)
////        .SetFrameBuffer(_desc.frameBuffer)
////        .SetViewport(_desc.vp)
////        .SetGfxState(g_basicRendererCtx->lightVolumeState)
////        .SetBlendState(BlendState::kAdditive, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(Executor {});
////
////    if (_desc.shadowMap)
////        pass.AddTexture(_desc.shadowMap, eShader::PixelShader, t_shadowSlot);
////
////    return pass;
////}
////
//// RenderPass BasicRenderer::CreateSkyboxPass(const SkyboxPassDesc& _desc)
////{
////    struct SkyboxExecutor
////    {
////        void operator()() const
////        {
////            g_basicRendererCtx->skyboxCube.Submit();
////        }
////    };
////
////    RenderPass pass;
////    pass.SetProgram(g_basicRendererCtx->skyboxProgram)
////        .AddSampler(g_basicRendererCtx->linearClampSampler, eShader::PixelShader, s_linearClampSlot)
////        .AddConstantBuffer(g_basicRendererCtx->cameraCbh, eShader::VertexShader, CB_CAMERA_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
////        .SetFrameBuffer(_desc.frameBuffer)
////        .SetViewport(_desc.vp)
////        .SetGfxState(g_basicRendererCtx->skyboxState)
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(SkyboxExecutor {});
////
////    if (_desc.skyboxTexture)
////        pass.AddTexture(_desc.skyboxTexture, eShader::PixelShader, t_skyboxSlot);
////
////    return pass;
////}
////
//// CreateShadowCasterPassResult BasicRenderer::CreateShadowCasterPass(const ShadowCasterPassDesc& _desc)
////{
////    CreateShadowCasterPassResult result;
////
////    TextureHandle     th  = CreateTexture2D(_desc.lengthOfSide, _desc.lengthOfSide, eTextureFormat::D32_Float, eAttachment::DepthStencil);
////    FrameBufferHandle fbh = gfx.CreateFrameBuffer(th, true);
////    Viewport          vp { 0, 0, static_cast<UInt16>(_desc.lengthOfSide), static_cast<UInt16>(_desc.lengthOfSide) };
////
////    RenderPass& pass = result.pass;
////    pass.SetProgram(g_basicRendererCtx->shadowCasterProgram)
////        .AddConstantBuffer(g_basicRendererCtx->transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->shadowCasterCbh, eShader::VertexShader, CB_SHADOW_CASTER_SLOT)
////        .SetFrameBuffer(fbh, true)
////        .SetViewport(vp)
////        .SetGfxState(g_basicRendererCtx->depthTestState)
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(_desc.executor);
////
////    result.shadowMap = th;
////    return result;
////}
////
//// CreateShadowCasterPassResult BasicRenderer::CreateOmniDirectionalShadowCasterPass(const ShadowCasterPassDesc& _desc)
////{
////    CreateShadowCasterPassResult result;
////
////    TextureHandle     th  = CreateTextureCube(_desc.lengthOfSide, _desc.lengthOfSide, eTextureFormat::D32_Float, eAttachment::DepthStencil);
////    FrameBufferHandle fbh = gfx.CreateFrameBuffer(th, true);
////    Viewport          vp { 0, 0, static_cast<UInt16>(_desc.lengthOfSide), static_cast<UInt16>(_desc.lengthOfSide) };
////
////    RenderPass& pass = result.pass;
////    pass.SetProgram(g_basicRendererCtx->osmCasterProgram)
////        .AddConstantBuffer(g_basicRendererCtx->transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->osmCasterCbh, eShader::GeometryShader, CB_OMNIDIRECTIONAL_SHADOW_CASTER_SLOT)
////        .SetFrameBuffer(fbh, true)
////        .SetViewport(vp)
////        .SetGfxState(g_basicRendererCtx->depthTestState)
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(_desc.executor);
////
////    result.shadowMap = th;
////    return result;
////}
////
//// CreateShadowCasterPassResult BasicRenderer::CreateCascadeShadowCasterPass(const ShadowCasterPassDesc& _desc)
////{
////    CreateShadowCasterPassResult result;
////
////    TextureHandle     th  = CreateTexture2D(_desc.lengthOfSide, _desc.lengthOfSide, eTextureFormat::D32_Float, eAttachment::DepthStencil);
////    FrameBufferHandle fbh = gfx.CreateFrameBuffer(th, true);
////    Viewport          vp { 0, 0, static_cast<UInt16>(_desc.lengthOfSide), static_cast<UInt16>(_desc.lengthOfSide) };
////
////    RenderPass& pass = result.pass;
////    pass.AddConstantBuffer(g_basicRendererCtx->transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->csmCasterCbh, eShader::GeometryShader, CB_CASCADE_SHADOW_CASTER_SLOT)
////        .SetFrameBuffer(fbh, true)
////        .SetViewport(vp)
////        .SetGfxState(g_basicRendererCtx->depthTestState)
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(_desc.executor);
////
////    if (_desc.cascadeLevel == eCascadeLevel::level2)
////    {
////        pass.SetProgram(g_basicRendererCtx->csmLevel2CasterProgram);
////    }
////    else if (_desc.cascadeLevel == eCascadeLevel::level3)
////    {
////        pass.SetProgram(g_basicRendererCtx->csmLevel3CasterProgram);
////    }
////    else if (_desc.cascadeLevel == eCascadeLevel::level4)
////    {
////        pass.SetProgram(g_basicRendererCtx->csmLevel4CasterProgram);
////    }
////
////    result.shadowMap = th;
////    return result;
////}
////
//// CreateSSAOPassResult BasicRenderer::CreateSSAOPass(const SSAOPassDesc& _desc)
////{
////    CreateSSAOPassResult result;
////    const GBufferDesc&   gbufferDesc = _desc.gBufferDesc;
////
////    Int32    w = _desc.width;
////    Int32    h = _desc.height;
////    Viewport vp { 0, 0, static_cast<UInt16>(w), static_cast<UInt16>(h) };
////
////    // ssao
////    TextureHandle     ssaoTexh    = gfx.CreateTexture2D(w, h, eTextureFormat::R8_UNorm, 1, eTextureCreateFlags::RenderTarget);
////    FrameBufferHandle ssaoPassFbh = gfx.CreateFrameBuffer(ssaoTexh, true);
////
////    RenderPass& ssaoPass = result.passes[0];
////    ssaoPass.SetProgram(g_basicRendererCtx->ssaoProgram)
////        .AddSampler(g_basicRendererCtx->pointClampSampler, eShader::PixelShader, s_pointClampSlot)
////        .AddSampler(g_basicRendererCtx->linearWrapSampler, eShader::PixelShader, s_linearWrapSlot)
////        .AddConstantBuffer(g_basicRendererCtx->cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->ssaoCbh, eShader::PixelShader, CB_SSAO_KERNEL_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
////        .AddTexture(gbufferDesc.normalRoughnessTexture, eShader::PixelShader, t_gBufferNormalRoughnessSlot)
////        .AddTexture(gbufferDesc.depthTexture, eShader::PixelShader, t_depthSlot)
////        .AddTexture(g_basicRendererCtx->ssaoNoiseTexture, eShader::PixelShader, t_ssaoNoiseSlot)
////        .SetFrameBuffer(ssaoPassFbh, true)
////        .SetViewport(vp)
////        .SetGfxState(GfxState::Default())
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////
////    // horizontal blur
////    TextureHandle     hBlurTexh = gfx.CreateTexture2D(w, h, eTextureFormat::R8_UNorm, 1, eTextureCreateFlags::RenderTarget);
////    FrameBufferHandle hBlurFbh  = gfx.CreateFrameBuffer(hBlurTexh, true);
////
////    RenderPass& hBlurPass = result.passes[1];
////    hBlurPass.SetProgram(g_basicRendererCtx->ssaoBlurXProgram)
////        .AddTexture(ssaoTexh, eShader::PixelShader, t_input0Slot)
////        .SetFrameBuffer(hBlurFbh, true)
////        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////
////    // vertical blur
////    TextureHandle     vBlurTexh = gfx.CreateTexture2D(w, h, eTextureFormat::R8_UNorm, 1, eTextureCreateFlags::RenderTarget);
////    FrameBufferHandle vBlurFbh  = gfx.CreateFrameBuffer(vBlurTexh, true);
////
////    RenderPass& vBlurPass = result.passes[2];
////    vBlurPass.SetProgram(g_basicRendererCtx->ssaoBlurYProgram)
////        .AddTexture(vBlurTexh, eShader::PixelShader, t_input0Slot)
////        .SetFrameBuffer(vBlurFbh, true)
////        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////
////    result.ssaoTexture = vBlurTexh;
////    return result;
////}
////
//// CreatePostProcessPassResult BasicRenderer::CreatePostProcessToneMappingPass(const ToneMappingPassDesc& _desc)
////{
////    CreatePostProcessPassResult result;
////
////    TextureHandle     outputTexh = CreateTexture2D(_desc.width, _desc.height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget);
////    FrameBufferHandle outputFbh  = gfx.CreateFrameBuffer(outputTexh, true);
////    Viewport          vp { 0, 0, static_cast<UInt16>(_desc.width), static_cast<UInt16>(_desc.height) };
////
////    RenderPass& pass = result.pass;
////    pass.SetProgram(g_basicRendererCtx->toneMappingProgram)
////        .AddSampler(g_basicRendererCtx->linearClampSampler, eShader::PixelShader, s_linearClampSlot)
////        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input0Slot)
////        .AddConstantBuffer(g_basicRendererCtx->environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
////        .SetFrameBuffer(outputFbh, true)
////        .SetViewport(vp)
////        .SetGfxState(GfxState::Default())
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////
////    result.outputTexture = outputTexh;
////    return result;
////}
////
//// CreatePostProcessPassResult BasicRenderer::CreatePostProcessFXAAPass(const FXAAPassDesc& _desc)
////{
////    CreatePostProcessPassResult result;
////
////    TextureHandle     outputTexh = CreateTexture2D(_desc.width, _desc.height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget);
////    FrameBufferHandle outputFbh  = gfx.CreateFrameBuffer(outputTexh, true);
////    Viewport          vp { 0, 0, static_cast<UInt16>(_desc.width), static_cast<UInt16>(_desc.height) };
////
////    RenderPass& pass = result.pass;
////    pass.AddSampler(g_basicRendererCtx->linearClampSampler, eShader::PixelShader, s_linearClampSlot)
////        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input0Slot)
////        .SetFrameBuffer(outputFbh, true)
////        .SetViewport(vp)
////        .SetGfxState(GfxState::Default())
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////
////    switch (_desc.quality)
////    {
////        case eFxaaQuality::VeryLow:
////            pass.SetProgram(g_basicRendererCtx->fxaaVeryLowProgram);
////            break;
////        case eFxaaQuality::Low:
////            pass.SetProgram(g_basicRendererCtx->fxaaLowProgram);
////            break;
////        case eFxaaQuality::Medium:
////            pass.SetProgram(g_basicRendererCtx->fxaaMediumProgram);
////            break;
////        case eFxaaQuality::High:
////            pass.SetProgram(g_basicRendererCtx->fxaaHighProgram);
////            break;
////        case eFxaaQuality::VeryHigh:
////            pass.SetProgram(g_basicRendererCtx->fxaaVeryHighProgram);
////            break;
////        case eFxaaQuality::Ultra:
////            pass.SetProgram(g_basicRendererCtx->fxaaUltraProgram);
////        default:
////            JAM_UNREACHABLE;
////            break;
////    }
////
////    result.outputTexture = outputTexh;
////    return result;
////}
////
//// CreatePostProcessPassResult BasicRenderer::CreatePostProcessHeightFogPass(const HeightFogPassDesc& _desc)
////{
////    CreatePostProcessPassResult result;
////
////    TextureHandle     outputTexh = CreateTexture2D(_desc.width, _desc.height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget);
////    FrameBufferHandle outputFbh  = gfx.CreateFrameBuffer(outputTexh, true);
////    Viewport          vp { 0, 0, static_cast<UInt16>(_desc.width), static_cast<UInt16>(_desc.height) };
////
////    RenderPass& pass = result.pass;
////    pass.SetProgram(g_basicRendererCtx->fogProgram)
////        .AddSampler(g_basicRendererCtx->linearClampSampler, eShader::PixelShader, s_linearClampSlot)
////        .AddConstantBuffer(g_basicRendererCtx->cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
////        .AddConstantBuffer(g_basicRendererCtx->environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
////        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input0Slot)
////        .AddTexture(_desc.depthTexture, eShader::PixelShader, t_depthSlot)
////        .SetFrameBuffer(outputFbh, true)
////        .SetViewport(vp)
////        .SetGfxState(GfxState::Default())
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////
////    result.outputTexture = outputTexh;
////    return result;
////}
////
//// CreatePostProcessPassResult BasicRenderer::CreatePostProcessPassThroughPass(const PassThroughPassDesc& _desc)
////{
////    CreatePostProcessPassResult result;
////
////    TextureHandle     outputTexh = CreateTexture2D(_desc.width, _desc.height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget);
////    FrameBufferHandle outputFbh  = gfx.CreateFrameBuffer(outputTexh, true);
////    Viewport          vp { 0, 0, static_cast<UInt16>(_desc.width), static_cast<UInt16>(_desc.height) };
////
////    RenderPass& pass = result.pass;
////    pass.SetProgram(g_basicRendererCtx->passTexhroughProgram)
////        .AddSampler(g_basicRendererCtx->linearClampSampler, eShader::PixelShader, s_linearClampSlot)
////        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input0Slot)
////        .SetFrameBuffer(outputFbh, true)
////        .SetViewport(vp)
////        .SetGfxState(GfxState::Default())
////        .SetBlendState(BlendState::kOpaque, 0)
////        .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////
////    result.outputTexture = outputTexh;
////    return result;
////}
////
//// CreateBloomPassResult BasicRenderer::CreatePostProcessBloomPass(const BloomPassDesc& _desc)
////{
////    CreateBloomPassResult result;
////    result.passes.reserve(_desc.numBlurPasses * 2 + 1);
////
////    TextureHandle inputTexh = _desc.inputTexture;
////    for (Int32 i = 0; i < _desc.numBlurPasses; ++i)
////    {
////        Int32 downScaleLevel = i + 1;
////        Int32 w              = Max(1, _desc.width >> downScaleLevel);
////        Int32 h              = Max(1, _desc.height >> downScaleLevel);
////
////        TextureHandle     downTexh = CreateTexture2D(w, h, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget);
////        FrameBufferHandle downFbh  = gfx.CreateFrameBuffer(downTexh, true);
////        Viewport          vp { 0, 0, static_cast<UInt16>(w), static_cast<UInt16>(h) };
////
////        RenderPass pass;
////        if (i == 0)
////        {
////            pass.SetProgram(g_basicRendererCtx->bloomDownProgram)
////                .AddSampler(g_basicRendererCtx->linearClampSampler, eShader::PixelShader, s_linearClampSlot)
////                .AddTexture(inputTexh, eShader::PixelShader, t_input0Slot)
////                .SetFrameBuffer(downFbh, true)
////                .SetViewport(vp)
////                .SetGfxState(GfxState::Default())
////                .SetBlendState(BlendState::kOpaque, 0)
////                .SetStencil(StencilState::kDisable, StencilState::kDisable, 0)
////                .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////        }
////        else
////        {
////            pass.SetProgram(g_basicRendererCtx->bloomDownProgram)
////                .AddTexture(inputTexh, eShader::PixelShader, t_input0Slot)
////                .SetFrameBuffer(downFbh, true)
////                .SetViewport(vp)
////                .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////        }
////
////        result.passes.emplace_back(std::move(pass));
////        inputTexh = downTexh;
////    }
////
////    for (Int32 i = 0; i < _desc.numBlurPasses; ++i)
////    {
////        Int32 upScaleLevel = _desc.numBlurPasses - i - 1;
////        Int32 w            = Max(1, _desc.width >> upScaleLevel);
////        Int32 h            = Max(1, _desc.height >> upScaleLevel);
////
////        TextureHandle     upTexh = CreateTexture2D(w, h, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget);
////        FrameBufferHandle upFbh  = gfx.CreateFrameBuffer(upTexh, true);
////        Viewport          vp { 0, 0, static_cast<UInt16>(w), static_cast<UInt16>(h) };
////
////        RenderPass pass;
////        pass.SetProgram(g_basicRendererCtx->bloomUpProgram)
////            .AddTexture(inputTexh, eShader::PixelShader, t_input0Slot)
////            .SetFrameBuffer(upFbh, true)
////            .SetViewport(vp)
////            .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////        result.passes.emplace_back(std::move(pass));
////        inputTexh = upTexh;
////    }
////
////    // combine
////    TextureHandle     combineTexh = CreateTexture2D(_desc.width, _desc.height, eTextureFormat::RGBA8_UNorm, eAttachment::RenderTarget);
////    FrameBufferHandle combineFbh  = gfx.CreateFrameBuffer(combineTexh, true);
////    Viewport          vp { 0, 0, static_cast<UInt16>(_desc.width), static_cast<UInt16>(_desc.height) };
////
////    RenderPass pass;
////    pass.SetProgram(g_basicRendererCtx->bloomCombineProgram)
////        .AddTexture(inputTexh, eShader::PixelShader, t_input0Slot)
////        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input1Slot)
////        .SetFrameBuffer(combineFbh, true)
////        .SetViewport(vp)
////        .SetExecutor(Bind<&BasicRendererContext::SubmitFullScreenTri>(this));
////    result.passes.emplace_back(std::move(pass));
////    result.outputTexture = combineTexh;
////    return result;
////}
//
//}   // namespace Jam