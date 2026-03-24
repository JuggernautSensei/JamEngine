#pragma once
#include "CppHlslGlue.h"
// ReSharper disable CppClangTidyModernizeMacroToEnum

JAM_NAMESPACE_BEGIN

#define JAM_LIGHT_TYPE       JAM_UINT32
#define JAM_LIGHT_TYPE_NONE  (0)
#define JAM_LIGHT_TYPE_POINT (1)
#define JAM_LIGHT_TYPE_SPOT  (2)

#define JAM_MATERIAL_FLAGS                         JAM_UINT32
#define JAM_MATERIAL_FLAGS_NONE                    (0)
#define JAM_MATERIAL_FLAGS_ALBEDO                  (1 << 0)
#define JAM_MATERIAL_FLAGS_NORMAL                  (1 << 1)
#define JAM_MATERIAL_FLAGS_AO                      (1 << 2)
#define JAM_MATERIAL_FLAGS_EMISSIVE                (1 << 3)
#define JAM_MATERIAL_FLAGS_VERTEX_DISPLACEMENT_MAP (1 << 4)
#define JAM_MATERIAL_FLAGS_LIGHT_MAP               (1 << 5)
#define JAM_MATERIAL_FLAGS_FLIP_Y_NORMAL           (1 << 6)
#define JAM_MATERIAL_FLAGS_METALLIC_R              (1 << 8)
#define JAM_MATERIAL_FLAGS_METALLIC_G              (1 << 9)
#define JAM_MATERIAL_FLAGS_METALLIC_B              (1 << 10)
#define JAM_MATERIAL_FLAGS_METALLIC_ALL            (JAM_MATERIAL_FLAGS_METALLIC_R | JAM_MATERIAL_FLAGS_METALLIC_G | JAM_MATERIAL_FLAGS_METALLIC_B)
#define JAM_MATERIAL_FLAGS_ROUGHNESS_R             (1 << 11)
#define JAM_MATERIAL_FLAGS_ROUGHNESS_G             (1 << 12)
#define JAM_MATERIAL_FLAGS_ROUGHNESS_B             (1 << 13)
#define JAM_MATERIAL_FLAGS_ROUGHNESS_ALL           (JAM_MATERIAL_FLAGS_ROUGHNESS_R | JAM_MATERIAL_FLAGS_ROUGHNESS_G | JAM_MATERIAL_FLAGS_ROUGHNESS_B)
#define JAM_MATERIAL_FLAGS_ALPHA_CUTOUT            (1 << 14)

#define JAM_ENVIRONMENT_FLAGS                    JAM_UINT32
#define JAM_ENVIRONMENT_FLAGS_NONE               (0)
#define JAM_ENVIRONMENT_FLAGS_SSAO               (1 << 0)
#define JAM_ENVIRONMENT_FLAGS_DIRLIGHT_SHADOW    (1 << 1)
#define JAM_ENVIRONMENT_FLAGS_SKYBOX_CUBEMAP     (1 << 2)
#define JAM_ENVIRONMENT_FLAGS_SKYBOX_SOLID_COLOR (1 << 3)

#define JAM_ENVIRONMENT_TONE_MAPPING_TYPE                           JAM_UINT32
#define JAM_ENVIRONMENT_TONE_MAPPING_TYPE_LINEAR                    (0)
#define JAM_ENVIRONMENT_TONE_MAPPING_TYPE_REINHARD                  (1)
#define JAM_ENVIRONMENT_TONE_MAPPING_TYPE_LUMA_BASED_REINHARD       (2)
#define JAM_ENVIRONMENT_TONE_MAPPING_TYPE_WHITE_PRESERVING_REINHARD (3)
#define JAM_ENVIRONMENT_TONE_MAPPING_TYPE_ROMB_DA_HOUSE             (4)
#define JAM_ENVIRONMENT_TONE_MAPPING_TYPE_UNCHARTED2                (5)
#define JAM_ENVIRONMENT_TONE_MAPPING_TYPE_FILMIC                    (6)
#define JAM_ENVIRONMENT_TONE_MAPPING_TYPE_NONE                      (7)

#define JAM_CASCADE_LEVEL    3
#define JAM_SSAO_KERNEL_SIZE 16

//===================================================
// Constant Buffers
//===================================================

JAM_CBUFFER(CBTransform, 0)
{
    JAM_MATRIX cb_world;
    JAM_MATRIX cb_worldInvTrans;
};

JAM_CBUFFER(CBCamera, 1)
{
    JAM_MATRIX cb_view;          // 64
    JAM_MATRIX cb_proj;          // 64
    JAM_MATRIX cb_viewProjInv;   // 64
    //
    JAM_FLOAT3 cb_eye;   // 12
    JAM_PAD_FLOAT;       // 4
};

JAM_CBUFFER(CBLight, 2)
{
    JAM_LIGHT_TYPE cb_lightType;   // 4
    JAM_FLOAT3     cb_lightPos;    // 12
    //
    JAM_FLOAT3 cb_lightDir;   // 12
    JAM_PAD_FLOAT;            // 4
    //
    JAM_FLOAT3 cb_lightRadiance;    // 12
    JAM_FLOAT  cb_lightIntensity;   // 4
    //
    JAM_FLOAT cb_lightFallOffStart;       // 4
    JAM_FLOAT cb_lightFallOffEnd;         // 4
    JAM_FLOAT cb_lightInnerConeHalfRad;   // 4
    JAM_FLOAT cb_lightOuterConeHalfRad;   // 4
    //
    JAM_BOOL cb_bShadowMapping;   // 4
    JAM_PAD_FLOAT3;               // 12
};

JAM_CBUFFER(CBMaterial, 3)
{
    // phong
    JAM_FLOAT3 cb_ambient;                 // 12
    JAM_FLOAT  cb_lightTextureIntencity;   // 4

    JAM_FLOAT3 cb_diffuse;                    // 12
    JAM_FLOAT  cb_emissiveTextureIntensity;   // 4

    JAM_FLOAT3 cb_specular;    // 12
    JAM_FLOAT  cb_shininess;   // 4

    // pbr
    JAM_FLOAT3 cb_albedo;      // 12
    JAM_FLOAT  cb_roughness;   // 4

    JAM_FLOAT3 cb_emissive;            // 12
    JAM_FLOAT  cb_emissiveIntensity;   // 4

    JAM_FLOAT          cb_metallic;                 // 4
    JAM_FLOAT          cb_displacementMapScaling;   // 4
    JAM_MATERIAL_FLAGS cb_materialFlags;            // 4
    JAM_PAD_FLOAT;                                  // 4
};

JAM_CBUFFER(CBEnvironment, 4)
{
    JAM_ENVIRONMENT_FLAGS cb_envFlags;            // 4
    JAM_FLOAT             cb_dirLightIntensity;   // 4
    JAM_FLOAT             cb_shadowBias;          // 4
    JAM_FLOAT             cb_ssaoBias;            // 4

    JAM_FLOAT3 cb_dirLightDirection;   // 12
    JAM_PAD_FLOAT;                     // 4

    JAM_FLOAT3 cb_dirLightRadiance;   // 12
    JAM_PAD_FLOAT;                    // 4

    JAM_FLOAT3 cb_cascadeShadowDistances;   // 12
    JAM_PAD_FLOAT;                          // 4

    JAM_FLOAT  cb_irradianceMapIntensity;   // 4
    JAM_FLOAT  cb_specularMapIntensity;     // 4
    JAM_UINT32 cb_specularMapNumMips;       // 4
    JAM_PAD_FLOAT;                          // 4

    JAM_FLOAT3 cb_skyBoxSolidColor;   // 12
    JAM_FLOAT  cb_ssaoRadius;         // 4

    JAM_FLOAT3 cb_fogColor;     // 12
    JAM_FLOAT  cb_fogDensity;   // 4

    JAM_FLOAT                         cb_fogFallOffStart;        // 4
    JAM_FLOAT                         cb_fogFallOffEnd;          // 4
    JAM_FLOAT                         cb_fogHeightAttenuation;   // 4
    JAM_ENVIRONMENT_TONE_MAPPING_TYPE cb_toneMappingType;        // 4

    JAM_FLOAT cb_exposure;          // 4
    JAM_FLOAT cb_gamma;             // 4
    JAM_FLOAT cb_bloomBlurRadius;   // 4
    JAM_FLOAT cb_bloomStrength;     // 4
};

JAM_CBUFFER(CBShadowReceiver, 5)
{
    JAM_MATRIX cb_shadowReceiverViewProj;                   // 64 (shadow map)
    JAM_MATRIX cb_csmReceiverViewProj[JAM_CASCADE_LEVEL];   // 64 (cascade shadow map)

    JAM_FLOAT cb_cascadeRanges[JAM_CASCADE_LEVEL];   // 16

    JAM_UINT32 cb_cascadeLevel;   // 4
    JAM_PAD_FLOAT3;               // 12
};

JAM_CBUFFER(CBScreen, 6)
{
    JAM_FLOAT cb_screenWidth;    // 4
    JAM_FLOAT cb_screenHeight;   // 4
    JAM_FLOAT cb_texelX;         // 4
    JAM_FLOAT cb_texelY;         // 4
};

JAM_CBUFFER(CBShadowCaster, 7)
{
    JAM_MATRIX cb_shaderMappingViewProj;
};

JAM_CBUFFER(CBCascadeShadowCaster, 8)
{
    JAM_MATRIX cb_csmViewProj[JAM_CASCADE_LEVEL];
};

JAM_CBUFFER(CBOmnidirectionalShadowCaster, 9)
{
    JAM_MATRIX cb_odsViewProj[6];
};

// 최대 8x8 커널
JAM_CBUFFER(CBSSAOKernel, 10)
{
    JAM_FLOAT4 cb_ssaoKernel[JAM_SSAO_KERNEL_SIZE];
};

//===================================================
// Vertex Structures
//===================================================

struct VSInputPosTex2D
{
    JAM_FLOAT2 posL     JAM_SEMANTIC(POSITION);
    JAM_FLOAT2 texCoord JAM_SEMANTIC(TEXCOORD0);
};

struct VSInputStaticMesh
{
    JAM_FLOAT3 posL      JAM_SEMANTIC(POSITION);
    JAM_FLOAT3 normalL   JAM_SEMANTIC(NORMAL);
    JAM_FLOAT3 tangentL  JAM_SEMANTIC(TANGENT);
    JAM_FLOAT2 texCoord0 JAM_SEMANTIC(TEXCOORD0);
    JAM_FLOAT2 texCoord1 JAM_SEMANTIC(TEXCOORD1);
};

struct VSInputPos3D
{
    JAM_FLOAT3 posL JAM_SEMANTIC(POSITION);
};

//===================================================
// Shader Resources
//===================================================

// standard material [0~9]
JAM_SHADER_RESOURCE_TEXTURE2D(t_displacementMap, 0);   // use in vertex shader
JAM_SHADER_RESOURCE_TEXTURE2D(t_albedo, 0);
JAM_SHADER_RESOURCE_TEXTURE2D(t_normal, 1);
JAM_SHADER_RESOURCE_TEXTURE2D(t_ao, 2);
JAM_SHADER_RESOURCE_TEXTURE2D(t_metallic, 3);
JAM_SHADER_RESOURCE_TEXTURE2D(t_roughness, 4);
JAM_SHADER_RESOURCE_TEXTURE2D(t_emissive, 5);
JAM_SHADER_RESOURCE_TEXTURE2D(t_lightmap, 6);
JAM_SHADER_RESOURCE_TEXTURE2D(t_ssao, 7);

// scene environment [10~14]
JAM_SHADER_RESOURCE_TEXTURECUBE(t_skybox, 10);
JAM_SHADER_RESOURCE_TEXTURECUBE(t_irradiance, 11);
JAM_SHADER_RESOURCE_TEXTURECUBE(t_specular, 12);
JAM_SHADER_RESOURCE_TEXTURE2D(t_brdf, 13);

// G-Buffer [15~19]
JAM_SHADER_RESOURCE_TEXTURE2D(t_gBufferAlbedo, 15);     // RGBA8_UNorm (albedo.rgb, A: alpha)
JAM_SHADER_RESOURCE_TEXTURE2D(t_gBufferMaterial, 16);   // RGBA8_UNorm (R: metallic, G: roughness, B: ao, A: misc)
JAM_SHADER_RESOURCE_TEXTURE2D(t_gBufferNormal, 17);     // RGB10A2_UNorm (normal.xyz, A: unused)
JAM_SHADER_RESOURCE_TEXTURE2D(t_gBufferLighting, 18);   // RG11B10_Float (emissive + gi + ...)

// Shadow [20 ~ 24]
JAM_SHADER_RESOURCE_TEXTURE2D(t_shadow, 20);
JAM_SHADER_RESOURCE_TEXTURECUBE(t_omniDirectionalShadow, 21);
JAM_SHADER_RESOURCE_TEXTURE2D_ARRAY(t_cascadeShadowArray, 22);

// SSAO [25 ~ 26]
JAM_SHADER_RESOURCE_TEXTURE2D(t_ssaoKernel, 25);
JAM_SHADER_RESOURCE_TEXTURE2D(t_ssaoNoise, 26);

// common [25 ~ 29]
JAM_SHADER_RESOURCE_TEXTURE2D(t_input0, 25);
JAM_SHADER_RESOURCE_TEXTURE2D(t_input1, 26);
JAM_SHADER_RESOURCE_TEXTURE2D(t_depth, 27);

//===================================================
// Resource Sampler
//===================================================

// 고정 샘플러
JAM_SHADER_RESOURCE_SAMPLER(s_linearWrap, 0);
JAM_SHADER_RESOURCE_SAMPLER(s_linearClamp, 1);
JAM_SHADER_RESOURCE_SAMPLER(s_pointClamp, 2);
JAM_SHADER_RESOURCE_SAMPLER(s_anisotropic4Wrap, 3);
JAM_SHADER_RESOURCE_SAMPLER(s_shadowLinearClamp, 4);

// 가변 샘플러
JAM_SHADER_RESOURCE_SAMPLER(s_sampler0, 10);
JAM_SHADER_RESOURCE_SAMPLER(s_sampler1, 11);

JAM_NAMESPACE_END
