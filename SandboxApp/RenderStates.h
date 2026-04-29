#pragma once

constexpr Flags<eSamplerFlags> kLinearWrap =
    eSamplerFlags::Filter_MinLinear_MagLinear_MipLinear
    | eSamplerFlags::AddressU_Wrap
    | eSamplerFlags::AddressV_Wrap
    | eSamplerFlags::AddressW_Wrap;

constexpr Flags<eSamplerFlags> kLinearClamp =
    eSamplerFlags::Filter_MinLinear_MagLinear_MipLinear
    | eSamplerFlags::AddressU_Clamp
    | eSamplerFlags::AddressV_Clamp
    | eSamplerFlags::AddressW_Clamp;

////////// RENDER FLAGS /////////////////////////////////////////////////////////////////////////

constexpr Flags<eRenderFlags> kBackCullDepthTest =
    eRenderFlags::CullMode_Back
    | eRenderFlags::DepthFunc_Less
    | eRenderFlags::DepthWrite;

constexpr Flags<eRenderFlags> kFrontCullDepthTest =
    eRenderFlags::CullMode_Front
    | eRenderFlags::DepthFunc_Less
    | eRenderFlags::DepthWrite;

constexpr Flags<eRenderFlags> kFrontCull =
    eRenderFlags::CullMode_Front;

constexpr Flags<eRenderFlags> kBackCull =
    eRenderFlags::CullMode_Back;

constexpr Flags<eRenderFlags> kSkyboxRenderFlags =
    eRenderFlags::DepthFunc_LessEqual
    | eRenderFlags::DepthWrite
    | eRenderFlags::CullMode_Front;

////////// BLEND STATE /////////////////////////////////////////////////////////////////////////

constexpr Flags<eBlendFlags> kOpaqueBlend =
    eBlendFlags::WriteMask_All;

constexpr Flags<eBlendFlags> kAlphaBlend =
    eBlendFlags::Enable
    | eBlendFlags::WriteMask_All
    | eBlendFlags::SrcBlend_SrcAlpha
    | eBlendFlags::DstBlend_InvSrcAlpha
    | eBlendFlags::BlendOp_Add
    | eBlendFlags::SrcBlendAlpha_SrcAlpha
    | eBlendFlags::DstBlendAlpha_InvSrcAlpha
    | eBlendFlags::BlendOpAlpha_Add;

constexpr Flags<eBlendFlags> kAdditiveBlend =
    eBlendFlags::Enable
    | eBlendFlags::WriteMask_All
    | eBlendFlags::SrcBlend_SrcAlpha
    | eBlendFlags::DstBlend_One
    | eBlendFlags::BlendOp_Add
    | eBlendFlags::SrcBlendAlpha_SrcAlpha
    | eBlendFlags::DstBlendAlpha_One
    | eBlendFlags::BlendOpAlpha_Add;