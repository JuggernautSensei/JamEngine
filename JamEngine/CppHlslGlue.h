#pragma once
// ReSharper disable CppClangTidyModernizeMacroToEnum

#ifdef JAM_CPP

#    define JAM_NAMESPACE_BEGIN \
        namespace Jam           \
        {

#    define JAM_NAMESPACE_END }

#    define JAM_CBUFFER(_name, _slot)              \
        constexpr __int32 k_##_name##Slot = _slot; \
        struct alignas(16) _name

#    define JAM_FLOAT  float
#    define JAM_FLOAT2 Vec2
#    define JAM_FLOAT3 Vec3
#    define JAM_FLOAT4 Vec4
#    define JAM_MATRIX Mtx4
#    define JAM_INT32  __int32
#    define JAM_UINT32 unsigned __int32
#    define JAM_BOOL   __int32

#    define JAM_TRUE  1
#    define JAM_FALSE 0

#    define JAM_PAD_IMPL(a, b)    a##b
#    define JAM_PAD(_type, _line) _type JAM_PAD_IMPL(cb_pad, _line)
#    define JAM_PAD_FLOAT         JAM_PAD(JAM_FLOAT, __LINE__)
#    define JAM_PAD_FLOAT2        JAM_PAD(JAM_FLOAT2, __LINE__)
#    define JAM_PAD_FLOAT3        JAM_PAD(JAM_FLOAT3, __LINE__)

#    define JAM_SEMANTIC(_sementic)

#    define JAM_SHADER_RESOURCE_TEXTURE2D(_name, _slot) \
        constexpr __int32 k_##_name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_TEXTURE2D_ARRAY(_name, _slot) \
        constexpr __int32 k_##_name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_TEXTURECUBE(_name, _slot) \
        constexpr __int32 k_##_name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_SAMPLER(_name, _slot) \
        constexpr __int32 k_##_name##Slot = _slot;

#    define JAM_SHADER_RESOURCE_SAMPLER_COMPARISON(_name, _slot) \
        constexpr __int32 k_##_name##Slot = _slot;

#else

#    define JAM_NAMESPACE_BEGIN
#    define JAM_NAMESPACE_END

#    define JAM_CBUFFER(_name, _slot) \
        cbuffer _name : register(b##_slot)

#    define JAM_FLOAT  float
#    define JAM_FLOAT2 float2
#    define JAM_FLOAT3 float3
#    define JAM_FLOAT4 float4
#    define JAM_MATRIX float4x4
#    define JAM_INT32  int
#    define JAM_UINT32 uint
#    define JAM_BOOL   JAM_UINT32

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
