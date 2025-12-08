#include "pch.h"

#include "Environment.h"

namespace jam
{

const DirectionalLightData& Environment::GetDirectionalLight() const
{
    return m_dirLight;
}

void Environment::SetDirectionalLight(const DirectionalLightData& _dirLight)
{
    m_dirLight = _dirLight;
    m_bDirty   = true;
}

const ImageBasedLightingData& Environment::GetIBL() const
{
    return m_ibl;
}

void Environment::SetIBL(const ImageBasedLightingData& _ibl)
{
    m_ibl    = _ibl;
    m_bDirty = true;
}

const SkyboxData& Environment::GetSkybox() const
{
    return m_skybox;
}

void Environment::SetSkybox(const SkyboxData& _skybox)
{
    m_skybox = _skybox;
    m_bDirty = true;
}

const SSAOData& Environment::GetSSAO() const
{
    return m_ssao;
}

void Environment::SetSSAO(const SSAOData& _ssao)
{
    m_ssao   = _ssao;
    m_bDirty = true;
}

const FogData& Environment::GetFog() const
{
    return m_fog;
}

void Environment::SetFog(const FogData& _fog)
{
    m_fog    = _fog;
    m_bDirty = true;
}

const ToneMappingData& Environment::GetToneMapping() const
{
    return m_toneMapping;
}

void Environment::SetToneMapping(const ToneMappingData& _toneMapping)
{
    m_toneMapping = _toneMapping;
    m_bDirty      = true;
}

const BloomData& Environment::GetBloom() const
{
    return m_bloom;
}

void Environment::SetBloom(const BloomData& _bloom)
{
    m_bloom  = _bloom;
    m_bDirty = true;
}

const FXAAData& Environment::GetFXAA() const
{
    return m_fxaa;
}

void Environment::SetFXAA(const FXAAData& _fxaa)
{
    m_fxaa   = _fxaa;
    m_bDirty = true;
}

bool Environment::IsDirty() const
{
    return m_bDirty;
}

void Environment::ClearDirty()
{
    m_bDirty = false;
}

inline const EnvironmentMisc& Environment::GetMisc() const
{
    return m_misc;
}

inline void Environment::SetMisc(const EnvironmentMisc& _misc)
{
    m_misc   = _misc;
    m_bDirty = true;
}

}   // namespace jam