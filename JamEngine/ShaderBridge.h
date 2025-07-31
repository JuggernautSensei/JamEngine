#ifndef JAM_SHADERBRIDGE_H
#define JAM_SHADERBRIDGE_H

#ifdef __cplusplus

#    define JAM_NAMESPACE_BEGIN \
        namespace jam           \
        {

#    define JAM_NAMESPACE_END }

#    define JAM_CBUFFER(_name, _slot)          \
        constexpr UInt32 _name##_SLOT = _slot; \
        struct alignas(16) _name

#    define JAM_FLOAT  float
#    define JAM_FLOAT2 Vec2
#    define JAM_FLOAT3 Vec3
#    define JAM_FLOAT4 Vec4
#    define JAM_MATRIX Mat4
#    define JAM_UINT32 UInt32

#    define JAM_SEMANTIC(_sementic)

#    define JAM_SHADER_RESOURCE_TEXTURE2D(_name, _slot) \
        constexpr UInt32 k_##_name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_TEXTURE2D_ARRAY(_name, _slot) \
        constexpr UInt32 k_##_name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_TEXTURECUBE(_name, _slot) \
        constexpr UInt32 k_##_name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_SAMPLER(_name, _slot) \
        constexpr UInt32 k_##_name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_SAMPLER_COMPARISON(_name, _slot) \
        constexpr UInt32 k_##_name##Slot = _slot;

#else

#    define JAM_NAMESPACE_BEGIN
#    define JAM_NAMESPACE_END

#    define JAM_CBUFFER(_name, _slot) \
        cbuffer _name : register(b##_slot)

#    define JAM_FLOAT  float
#    define JAM_FLOAT2 float2
#    define JAM_FLOAT3 float3
#    define JAM_FLOAT4 float4
#    define JAM_MATRIX row_major float4x4
#    define JAM_UINT32 uint

#    define JAM_SEMANTIC(_sementic) : _sementic

#    define JAM_SHADER_RESOURCE_TEXTURE2D(_name, _slot) \
        Texture2D _name : register(t##_slot)

#    define JAM_SHADER_RESOURCE_TEXTURE2D_ARRAY(_name, _slot) \
        Texture2DArray _name : register(t##_slot)

#    define JAM_SHADER_RESOURCE_TEXTURECUBE(_name, _slot) \
        TextureCube _name : register(t##_slot)

#    define JAM_SHADER_RESOURCE_SAMPLER(_name, _slot) \
        SamplerState _name : register(s##_slot)

#    define JAM_SHADER_RESOURCE_SAMPLER_COMPARISON(_name, _slot) \
        SamplerComparisonState _name : register(s##_slot)

#endif   // __cplusplus

JAM_NAMESPACE_BEGIN

#define JAM_LIGHT_TYPE             JAM_UINT32
#define JAM_LIGHT_TYPE_NONE        (0)
#define JAM_LIGHT_TYPE_POINT       (1)
#define JAM_LIGHT_TYPE_SPOT        (2)
#define JAM_LIGHT_TYPE_DIRECTIONAL (3)

#define JAM_MATERIAL_TEXTURE_BIND_FLAGS              JAM_UINT32
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_NONE         (0)
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_ALBEDO       (1 << 0)
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_NORMAL_GL    (1 << 1)
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_NORMAL_DX    (1 << 2)
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_METALLIC     (1 << 3)
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_ROUGHNESS    (1 << 4)
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_AO           (1 << 5)
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_EMISSIVE     (1 << 6)
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_DISPLACEMENT (1 << 7)
#define JAM_MATERIAL_TEXTURE_BIND_FLAGS_LIGHT_MAP    (1 << 8)

#define JAM_GLOBAL_RENDERING_FLAGS      JAM_UINT32
#define JAM_GLOBAL_RENDERING_FLAGS_NONE (0)
#define JAM_GLOBAL_RENDERING_FLAGS_SSAO (1 << 0)
#define JAM_GLOBAL_RENDERING_FLAGS_IBL  (1 << 1)

#define JAM_NUMBER_OF_LIGHT_CREATION_SHADOW     (1)
#define JAM_CASCADE_SHADOW_SPLIT_LEVEL          (3)   // if you want to use 4 splits, you have to set this value to 4
#define JAM_CASCADE_SHADOW_SPLIT_DISTANCE_COUNT (JAM_CASCADE_SHADOW_SPLIT_LEVEL - 1)

#define JAM_TOLERANCE (1e-6f)

//===================================================
// Constant Buffers
//===================================================

JAM_CBUFFER(CB_TRANSFORM, 0)
{
    JAM_MATRIX cb_transformWorldMat;
    JAM_MATRIX cb_transformWorldInvTransposeMat;   // World matrix
};

JAM_CBUFFER(CB_CAMERA, 1)
{
    JAM_MATRIX cb_cameraViewMat;
    JAM_MATRIX cb_cameraProjMat;
    JAM_MATRIX cb_cameraViewProjInvMat;
    JAM_FLOAT3 cb_cameraPosition;
    JAM_FLOAT  cb_cameraPad;
};

JAM_CBUFFER(CB_LIGHTING, 2)
{
    JAM_FLOAT3     cb_lightPosition;   // 12
    JAM_LIGHT_TYPE cb_lightType;       // 4
    // --------------------------
    JAM_FLOAT3 cb_lightDirection;   // 12
    JAM_FLOAT  cb_lightPad;         // 4
    // --------------------------
    JAM_FLOAT3 cb_lightRadiance;       // 12
    JAM_FLOAT  cb_lightFallOffStart;   // 4
    // --------------------------
    JAM_FLOAT cb_lightFallOffEnd;       // 4
    JAM_FLOAT cb_lightStrength;         // 4
    JAM_FLOAT cb_lightInnerConeAngle;   // 4
    JAM_FLOAT cb_lightOuterConeAngle;   // 4
};

JAM_CBUFFER(CB_MATERIAL, 3)
{
    JAM_FLOAT3 cb_materialAlbedo;     // 12
    JAM_FLOAT  cb_materialMetallic;   // 4
    // ----------------------
    JAM_FLOAT3 cb_materialEmission;           // 12
    JAM_FLOAT  cb_materialEmissionStrength;   // 4
    // ----------------------
    JAM_FLOAT  cb_materialRoughness;          // 4
    JAM_FLOAT  cb_materialLightmapStrength;   // 4
    JAM_FLOAT2 cb_materialPad;                // 8 (pad)
    // ----------------------
    JAM_FLOAT3 cb_materialDiffuse;                // 12
    JAM_FLOAT  cb_materialDisplacementStrength;   // 4
    // ----------------------
    JAM_FLOAT3 cb_materialSpecular;    // 12
    JAM_FLOAT  cb_materialSharpness;   // 4
    // ----------------------
    JAM_FLOAT3                      cb_materialAmbient;            // 12
    JAM_MATERIAL_TEXTURE_BIND_FLAGS cb_materialTextureBindFlags;   // 4
};

JAM_CBUFFER(CB_GLOBAL, 4)
{
    // common
    JAM_GLOBAL_RENDERING_FLAGS cb_globalRenderingFlags;         // 4
    JAM_FLOAT                  cb_globalSSAOStrength;           // 4 (for SSAO)
    JAM_FLOAT                  cb_globalIBLStrength;            // 4 (for IBL)
    JAM_FLOAT                  cb_globalScreenWidth;            // 4
    JAM_FLOAT                  cb_globalScreenHeight;           // 4
    JAM_FLOAT3                 cb_globalSceneAmbientRadiance;   // 12 (fot not-IBL)
};

JAM_CBUFFER(CB_POSTPROCESS, 5)
{
    // bloom
    JAM_FLOAT cb_postProcessBlurRadius;
    JAM_FLOAT cb_postProcessCombineStrength;

    // tone
    JAM_FLOAT cb_postProcessExposure;
    JAM_FLOAT cb_postProcessGamma;

    // fog
    JAM_FLOAT cb_postProcessFogDensity;
    JAM_FLOAT cb_postProcessFogOpacity;

    JAM_FLOAT cb_postProcessFogStartDistance;
    JAM_FLOAT cb_postProcessFogMaxOpacity;
    JAM_FLOAT cb_postProcessFogHeightFallOff;
    JAM_FLOAT cb_postProcessPad;

    JAM_FLOAT3 cb_postProcessFogColor;
    JAM_FLOAT  cb_postProcessFogHeight;
};

JAM_CBUFFER(CB_SHADOW_RECEIVER, 6)
{
    JAM_MATRIX cb_shadowReceiverViewProj[JAM_NUMBER_OF_LIGHT_CREATION_SHADOW];
    JAM_MATRIX cb_shadowReceiverCascadeShadowViewProj[JAM_CASCADE_SHADOW_SPLIT_LEVEL];

#if JAM_CASCADE_SHADOW_SPLIT_LEVEL == 2

    JAM_FLOAT  cb_shadowReceiverCascadeShadowRange1;
    JAM_FLOAT3 cb_shadowReceiverPad;

#elif JAM_CASCADE_SHADOW_SPLIT_LEVEL == 3

    JAM_FLOAT  cb_shadowReceiverCascadeShadowRange1;
    JAM_FLOAT  cb_shadowReceiverCascadeShadowRange2;
    JAM_FLOAT2 cb_shadowReceiverPad;

#elif JAM_CASCADE_SHADOW_SPLIT_LEVEL == 4

    JAM_FLOAT cb_shadowReceiverCascadeShadowRange1;
    JAM_FLOAT cb_shadowReceiverCascadeShadowRange2;
    JAM_FLOAT cb_shadowReceiverCascadeShadowRange3;
    JAM_FLOAT cb_shadowReceiverPad;

#else
#    error "CASCADE_SHADOW_SPLIT_LEVEL must be 2, 3 or 4"
#endif
};

JAM_CBUFFER(CB_SHADOW_CASTER, 1)
{
    JAM_MATRIX cb_shadowCasterView;
    JAM_MATRIX cb_shadowCasterProj;
};

JAM_CBUFFER(CB_CASCADE_SHADOW_CASTER, 1)
{
    JAM_MATRIX cb_cascadeShadowCasterViewMat[JAM_CASCADE_SHADOW_SPLIT_LEVEL];
    JAM_MATRIX cb_cascadeShadowCasterProjMat[JAM_CASCADE_SHADOW_SPLIT_LEVEL];
};

JAM_CBUFFER(CB_OMNIDIRECTIONAL_SHADOW_CASTER, 1)
{
    JAM_MATRIX cb_omniDirectionalShadowCasterViewProjMat[6];
    JAM_FLOAT3 cb_omniDirectionalShadowCasterLightPositionW;
    JAM_FLOAT  cb_omniDirectionalShadowCasterLightRange;
};

JAM_CBUFFER(CB_SSAO, 2)
{
    JAM_FLOAT cb_ssaoRadius;
    JAM_FLOAT cb_ssaoBias;
};

//===================================================
// Vertex Structures
//===================================================

struct VS_INPUT_VERTEX2
{
    JAM_FLOAT2 positionL JAM_SEMANTIC(POSITION);
    JAM_FLOAT2 uv0       JAM_SEMANTIC(TEXCOORD0);
};

struct VS_INPUT_VERTEX3
{
    JAM_FLOAT3 positionL JAM_SEMANTIC(POSITION);
    JAM_FLOAT3 normal    JAM_SEMANTIC(NORMAL);
    JAM_FLOAT2 uv0       JAM_SEMANTIC(TEXCOORD0);
    JAM_FLOAT2 uv1       JAM_SEMANTIC(TEXCOORD1);
    JAM_FLOAT3 tangentL  JAM_SEMANTIC(TANGENT);
};

struct VS_INPUT_VERTEX3_POSONLY
{
    JAM_FLOAT3 positionL JAM_SEMANTIC(POSITION);
};

//===================================================
// Resource Texture
//===================================================

// Texture [0~9]
JAM_SHADER_RESOURCE_TEXTURE2D(displacementTexture, 0);   // use in vertex shader
JAM_SHADER_RESOURCE_TEXTURE2D(albedoTexture, 0);
JAM_SHADER_RESOURCE_TEXTURE2D(normalTexture, 1);
JAM_SHADER_RESOURCE_TEXTURE2D(aoTexture, 2);
JAM_SHADER_RESOURCE_TEXTURE2D(metallicTexture, 3);
JAM_SHADER_RESOURCE_TEXTURE2D(roughnessTexture, 4);
JAM_SHADER_RESOURCE_TEXTURE2D(emissiveTexture, 5);
JAM_SHADER_RESOURCE_TEXTURE2D(lightmapTexture, 6);
JAM_SHADER_RESOURCE_TEXTURE2D(ssaoTexture, 7);

// IBL [10~14]
JAM_SHADER_RESOURCE_TEXTURECUBE(skyboxCubemap, 10);
JAM_SHADER_RESOURCE_TEXTURECUBE(diffuseCubemap, 11);
JAM_SHADER_RESOURCE_TEXTURECUBE(specularCubemap, 12);
JAM_SHADER_RESOURCE_TEXTURE2D(brdfTexture, 13);

// G-Buffer Light [16~19]
JAM_SHADER_RESOURCE_TEXTURE2D(gBufferNormalTexture, 15);
JAM_SHADER_RESOURCE_TEXTURE2D(gBufferAlbedoRoughnessTexture, 16);
JAM_SHADER_RESOURCE_TEXTURE2D(gBufferMetallicAOTexture, 17);
JAM_SHADER_RESOURCE_TEXTURE2D(gBufferEmissionTexture, 18);

// Depth [20~24]
JAM_SHADER_RESOURCE_TEXTURE2D(depthTexture, 20);

// Shadow [25~29]
JAM_SHADER_RESOURCE_TEXTURE2D(shadowMapTexture, 25);
JAM_SHADER_RESOURCE_TEXTURECUBE(omniShadowMapTexture, 26);
JAM_SHADER_RESOURCE_TEXTURE2D_ARRAY(cascadeShadowMapTextureArray, 27);

JAM_SHADER_RESOURCE_TEXTURE2D(ssaoNoiseTexture, 30);   // SSAO

// Image Filter [35~39]
JAM_SHADER_RESOURCE_TEXTURE2D(postProcessInputTexture1, 35);
JAM_SHADER_RESOURCE_TEXTURE2D(postProcessInputTexture2, 36);
JAM_SHADER_RESOURCE_TEXTURE2D(ssaoInputTexture, 37);

//===================================================
// Resource Sampler
//===================================================

JAM_SHADER_RESOURCE_SAMPLER(samplerLinearWrap, 0);
JAM_SHADER_RESOURCE_SAMPLER(samplerLinearClamp, 1);
JAM_SHADER_RESOURCE_SAMPLER(samplerPointWrap, 2);
JAM_SHADER_RESOURCE_SAMPLER(samplerPointClamp, 3);
JAM_SHADER_RESOURCE_SAMPLER(samplerLinearAnisotropic4Wrap, 5);
JAM_SHADER_RESOURCE_SAMPLER(samplerShadowComparisonLinearWrap, 6);

JAM_NAMESPACE_END

#endif   // JAM_SHADERBRIDGE_H
