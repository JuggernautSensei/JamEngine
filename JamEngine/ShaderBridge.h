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
#    define JAM_BOOL   UInt32

#    define JAM_TRUE  1
#    define JAM_FALSE 0

#    define JAM_PAD_IMPL(a, b)    a##b
#    define JAM_PAD(_type, _line) _type JAM_PAD_IMPL(cb_pad, _line)
#    define JAM_PAD_FLOAT         JAM_PAD(JAM_FLOAT, __LINE__)
#    define JAM_PAD_FLOAT2        JAM_PAD(JAM_FLOAT2, __LINE__)
#    define JAM_PAD_FLOAT3        JAM_PAD(JAM_FLOAT3, __LINE__)

#    define JAM_SEMANTIC(_sementic)

#    define JAM_SHADER_RESOURCE_TEXTURE2D(_name, _slot) \
        constexpr UInt32 _name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_TEXTURE2D_ARRAY(_name, _slot) \
        constexpr UInt32 _name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_TEXTURECUBE(_name, _slot) \
        constexpr UInt32 _name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_SAMPLER(_name, _slot) \
        constexpr UInt32 _name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_SAMPLER_COMPARISON(_name, _slot) \
        constexpr UInt32 _name##Slot = _slot;

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
#    define JAM_BOOL   UInt32

#    define JAM_TRUE  1
#    define JAM_FALSE 0

#    define JAM_PAD_IMPL(a, b)    a##b
#    define JAM_PAD(_type, _line) _type JAM_PAD_IMPL(cb_pad, _line)
#    define JAM_PAD_FLOAT         JAM_PAD(JAM_FLOAT, __LINE__)
#    define JAM_PAD_FLOAT2        JAM_PAD(JAM_FLOAT2, __LINE__)
#    define JAM_PAD_FLOAT3        JAM_PAD(JAM_FLOAT3, __LINE__)

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

#define JAM_MATERIAL_FLAGS               JAM_UINT32
#define JAM_MATERIAL_FLAGS_NONE          (0)
#define JAM_MATERIAL_FLAGS_ALBEDO        (1 << 0)
#define JAM_MATERIAL_FLAGS_NORMAL        (1 << 1)
#define JAM_MATERIAL_FLAGS_METALLIC      (1 << 3)
#define JAM_MATERIAL_FLAGS_ROUGHNESS     (1 << 4)
#define JAM_MATERIAL_FLAGS_AO            (1 << 5)
#define JAM_MATERIAL_FLAGS_EMISSIVE      (1 << 6)
#define JAM_MATERIAL_FLAGS_DISPLACEMENT  (1 << 7)
#define JAM_MATERIAL_FLAGS_LIGHT_MAP     (1 << 8)
#define JAM_MATERIAL_FLAGS_NORMAK_FLIP_Y (1 << 9)

#define JAM_SHADOW_CAST_LIGHT_COUNT    (1)
#define JAM_CASCADE_SHADOW_SPLIT_LEVEL (3)   // 2 ~ 4

//===================================================
// Constant Buffers
//===================================================

JAM_CBUFFER(CB_TRANSFORM, 0)
{
    JAM_MATRIX cb_world;
    JAM_MATRIX cb_worldInvTrans;   // World matrix
};

JAM_CBUFFER(CB_CAMERA, 1)
{
    JAM_MATRIX cb_view;          // 64
    JAM_MATRIX cb_proj;          // 64
    JAM_MATRIX cb_viewProjInv;   // 64
    //
    JAM_FLOAT3 cb_eyePos;   // 12
    JAM_PAD_FLOAT;          // 4
};

JAM_CBUFFER(CB_LIGHTING, 2)
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
    JAM_FLOAT cb_lightFallOffStart;     // 4
    JAM_FLOAT cb_lightFallOffEnd;       // 4
    JAM_FLOAT cb_lightInnerConeAngle;   // 4
    JAM_FLOAT cb_lightOuterConeAngle;   // 4
};

JAM_CBUFFER(CB_MATERIAL, 3)
{
    JAM_FLOAT3 cb_albedo;     // 12
    JAM_FLOAT  cb_metallic;   // 4
    //
    JAM_FLOAT3 cb_emissive;         // 12
    JAM_FLOAT  cb_emissiveFactor;   // 4
    //
    JAM_FLOAT  cb_roughness;   // 4
    JAM_FLOAT3 cb_diffuse;     // 12
    //
    JAM_FLOAT3 cb_specular;    // 12
    JAM_FLOAT  cb_sharpness;   // 4
    //
    JAM_FLOAT3         cb_ambient;         // 12
    JAM_MATERIAL_FLAGS cb_materialFlags;   // 4
};

JAM_CBUFFER(CB_GLOBAL, 4)
{
    // [common]
    JAM_FLOAT2 cb_screenSize;   // 8
    JAM_FLOAT2 cb_texelSize;    // 8
    //
    JAM_FLOAT cb_deltaTime;       // 4
    JAM_FLOAT cb_blurRadius;      // 4
    JAM_FLOAT cb_combineFactor;   // 4
    JAM_FLOAT cb_exposure;        // 4
    //
    JAM_FLOAT cb_gamma;              // 4
    JAM_FLOAT cb_fogDensity;         // 4
    JAM_FLOAT cb_fogOpacity;         // 4
    JAM_FLOAT cb_fogStartDistance;   // 4
    //
    JAM_FLOAT cb_fogMaxOpacity;      // 4
    JAM_FLOAT cb_fogHeightFallOff;   // 4
    JAM_FLOAT cb_fogHeight;          // 4
    JAM_PAD_FLOAT;                   // 4
    //
    JAM_FLOAT3 cb_fogColor;   // 12
    JAM_PAD_FLOAT;            // 4
    //
    JAM_FLOAT cb_ssaoRadius;   // 4
    JAM_FLOAT cb_ssaoBias;     // 4
    JAM_FLOAT cb_ssaoFactor;   // 4
    JAM_PAD_FLOAT;             // 4
};

JAM_CBUFFER(CB_SHADOW_RECEIVER, 5)
{
    JAM_MATRIX cb_shadowReceiverViewProj[JAM_SHADOW_CAST_LIGHT_COUNT];                   // 64 * JAM_SHADOW_CAST_LIGHT_COUNT
    JAM_MATRIX cb_shadowReceiverCascadeShadowViewProj[JAM_CASCADE_SHADOW_SPLIT_LEVEL];   // 64 * JAM_CASCADE_SHADOW_SPLIT_LEVEL

#if JAM_CASCADE_SHADOW_SPLIT_LEVEL == 2

    JAM_FLOAT cb_shadowReceiverCascadeShadowRange1;   // 4
    JAM_PAD_FLOAT3;                                   // 12

#elif JAM_CASCADE_SHADOW_SPLIT_LEVEL == 3

    JAM_FLOAT cb_shadowReceiverCascadeShadowRange1;   // 4
    JAM_FLOAT cb_shadowReceiverCascadeShadowRange2;   // 4
    JAM_PAD_FLOAT2;                                   // 8

#elif JAM_CASCADE_SHADOW_SPLIT_LEVEL == 4

    JAM_FLOAT cb_shadowReceiverCascadeShadowRange1;   // 4
    JAM_FLOAT cb_shadowReceiverCascadeShadowRange2;   // 4
    JAM_FLOAT cb_shadowReceiverCascadeShadowRange3;   // 4
    JAM_PAD_FLOAT;                                    // 4

#else
#    error "CASCADE_SHADOW_SPLIT_LEVEL must be 2, 3 or 4"
#endif
};

JAM_CBUFFER(CB_SHADOW_CASTER, 6)
{
    JAM_MATRIX cb_shadowCasterView;
    JAM_MATRIX cb_shadowCasterProj;
};

JAM_CBUFFER(CB_CASCADE_SHADOW_CASTER, 7)
{
    JAM_MATRIX cb_cascadeShadowCasterViewMat[JAM_CASCADE_SHADOW_SPLIT_LEVEL];
    JAM_MATRIX cb_cascadeShadowCasterProjMat[JAM_CASCADE_SHADOW_SPLIT_LEVEL];
};

JAM_CBUFFER(CB_OMNIDIRECTIONAL_SHADOW_CASTER, 8)
{
    JAM_MATRIX cb_omniDirectionalShadowCasterViewProjMat[6];
    JAM_FLOAT3 cb_omniDirectionalShadowCasterLightPosition;
    JAM_FLOAT  cb_omniDirectionalShadowCasterLightRange;
};

//===================================================
// Vertex Structures
//===================================================

struct VS_INPUT_VERTEX2
{
    JAM_FLOAT2 position JAM_SEMANTIC(POSITION);
    JAM_FLOAT2 texCoord JAM_SEMANTIC(TEXCOORD0);
};

struct VS_INPUT_VERTEX3
{
    JAM_FLOAT3 position  JAM_SEMANTIC(POSITION);
    JAM_FLOAT3 normal    JAM_SEMANTIC(NORMAL);
    JAM_FLOAT2 texCoord0 JAM_SEMANTIC(TEXCOORD0);
    JAM_FLOAT2 texCoord1 JAM_SEMANTIC(TEXCOORD1);
    JAM_FLOAT3 tangent   JAM_SEMANTIC(TANGENT);
};

struct VS_INPUT_VERTEX3_POSONLY
{
    JAM_FLOAT3 position JAM_SEMANTIC(POSITION);
};

//===================================================
// Resource TextureRef
//===================================================

// standard material [0~9]
JAM_SHADER_RESOURCE_TEXTURE2D(t_displacement, 0);   // use in vertex shader
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
JAM_SHADER_RESOURCE_TEXTURECUBE(t_irradiate, 11);
JAM_SHADER_RESOURCE_TEXTURECUBE(t_specular, 12);
JAM_SHADER_RESOURCE_TEXTURE2D(t_brdf, 13);

// G-Buffer [15~19]
JAM_SHADER_RESOURCE_TEXTURE2D(t_gBufferNormal, 15);
JAM_SHADER_RESOURCE_TEXTURE2D(t_gBufferAlbedoRoughness, 16);
JAM_SHADER_RESOURCE_TEXTURE2D(t_gBufferMetallicAO, 17);
JAM_SHADER_RESOURCE_TEXTURE2D(t_gBufferEmission, 18);

// Shadow [20 ~ 24]
JAM_SHADER_RESOURCE_TEXTURE2D(t_shadow, 20);
JAM_SHADER_RESOURCE_TEXTURECUBE(t_omniDirectionalShadow, 21);
JAM_SHADER_RESOURCE_TEXTURE2D_ARRAY(t_cascadeShadowArray, 22);

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
JAM_SHADER_RESOURCE_SAMPLER(s_pointWrap, 2);
JAM_SHADER_RESOURCE_SAMPLER(s_pointClamp, 3);
JAM_SHADER_RESOURCE_SAMPLER(s_anisotropic4Wrap, 5);
JAM_SHADER_RESOURCE_SAMPLER(s_shadowComparisonLinearWrap, 6);

// 가변 샘플러
JAM_SHADER_RESOURCE_SAMPLER(s_sampler0, 10);
JAM_SHADER_RESOURCE_SAMPLER(s_sampler1, 11);

JAM_NAMESPACE_END

#endif   // JAM_SHADERBRIDGE_H