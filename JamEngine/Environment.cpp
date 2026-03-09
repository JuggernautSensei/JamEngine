#include "pch.h"

#include "Environment.h"

namespace Jam
{

void Environment::SetDirectionalLightEnabled(const bool _bEnabled)
{
    m_bDirLightEnabled = _bEnabled;
    m_bDirty           = true;
}

bool Environment::IsDirectionalLightEnabled() const
{
    return m_bDirLightEnabled;
}

void Environment::SetDirectionalLightRadiance(const Vector3 _radiance)
{
    m_dirLightRadiance = _radiance;
    m_bDirty           = true;
}

Vector3 Environment::GetDirectionalLightRadiance() const
{
    return m_dirLightRadiance;
}

void Environment::SetDirectionalLightIntensity(const float _intensity)
{
    m_dirLightIntensity = _intensity;
    m_bDirty            = true;
}

float Environment::GetDirectionalLightIntensity() const
{
    return m_dirLightIntensity;
}

void Environment::SetDirectionalLightDirection(const Vector3 _direction)
{
    m_dirLightDirection = _direction;
    m_bDirty            = true;
}

Vector3 Environment::GetDirectionalLightDirection() const
{
    return m_dirLightDirection;
}

void Environment::SetDirectionalLightShadowEnabled(const bool _bEnabled)
{
    m_bDirLightShadowEnabled = _bEnabled;
    m_bDirty                 = true;
}

bool Environment::IsDirectionalLightShadowEnabled() const
{
    return m_bDirLightShadowEnabled;
}

void Environment::SetDirectionalLightShadowBias(const float _bias)
{
    m_dirLightShadowBias = _bias;
    m_bDirty             = true;
}

float Environment::GetDirectionalLightShadowBias() const
{
    return m_dirLightShadowBias;
}

void Environment::SetDirectionalLightCascadeDistances(const CascadeDistancesArray& _distances)
{
    if (JAM_DEBUG_ENABLED)
    {
        JAM_ASSERT(std::ranges::is_sorted(_distances, std::ranges::greater {}), "Cascade distances must be sorted in ascending order.");
    }

    m_dirLightCascadeDistances = _distances;
    m_bDirty                   = true;
}

const CascadeDistancesArray& Environment::GetDirectionalLightCascadeDistances() const
{
    return m_dirLightCascadeDistances;
}

void Environment::SetIblIrradianceTex(const Ref<Texture>& _pTexture)
{
    m_pIblIrradianceTex = _pTexture;
    m_bDirty            = true;
}

const Ref<Texture>& Environment::GetIblIrradianceTex() const
{
    return m_pIblIrradianceTex;
}

void Environment::SetIblSpecularTex(const Ref<Texture>& _pTexture)
{
    m_pIblSpecularTex = _pTexture;
    m_bDirty          = true;
}

const Ref<Texture>& Environment::GetIblSpecularTex() const
{
    return m_pIblSpecularTex;
}

void Environment::SetIblBrdfLutTex(const Ref<Texture>& _pTexture)
{
    m_pIblBrdfLutTex = _pTexture;
    m_bDirty         = true;
}

const Ref<Texture>& Environment::GetIblBrdfLutTex() const
{
    return m_pIblBrdfLutTex;
}

void Environment::SetIblIrradianceIntensity(const float _intensity)
{
    m_iblIrradianceTexIntensity = _intensity;
    m_bDirty                    = true;
}

float Environment::GetIblIrradianceIntensity() const
{
    return m_iblIrradianceTexIntensity;
}

void Environment::SetIblSpecularIntensity(const float _intensity)
{
    m_iblSpecularTexIntensity = _intensity;
    m_bDirty                  = true;
}

float Environment::GetIblSpecularIntensity() const
{
    return m_iblSpecularTexIntensity;
}

void Environment::SetSkyboxType(const eSkyboxType _type)
{
    m_skyboxType = _type;
    m_bDirty     = true;
}

eSkyboxType Environment::GetSkyboxType() const
{
    return m_skyboxType;
}

void Environment::SetSkyboxTex(const Ref<Texture>& _pTexture)
{
    m_pSkyboxTex = _pTexture;
    m_bDirty     = true;
}

const Ref<Texture>& Environment::GetSkyboxTex() const
{
    return m_pSkyboxTex;
}

void Environment::SetSkyboxSolidColor(const Vector3 _color)
{
    m_skyboxSolidColor = _color;
    m_bDirty           = true;
}

Vector3 Environment::GetSkyboxSolidColor() const
{
    return m_skyboxSolidColor;
}

void Environment::SetSsaoEnabled(const bool _bEnabled)
{
    m_bSsaoEnabled = _bEnabled;
    m_bDirty       = true;
}

bool Environment::IsSsaoEnabled() const
{
    return m_bSsaoEnabled;
}

void Environment::SetSsaoRadius(const float _radius)
{
    m_ssaoRadius = _radius;
    m_bDirty     = true;
}

float Environment::GetSsaoRadius() const
{
    return m_ssaoRadius;
}

void Environment::SetSsaoBias(const float _bias)
{
    m_ssaoBias = _bias;
    m_bDirty   = true;
}

float Environment::GetSsaoBias() const
{
    return m_ssaoBias;
}

void Environment::SetFogEnabled(const bool _bEnabled)
{
    m_bFogEnabled = _bEnabled;
    m_bDirty      = true;
}

bool Environment::IsFogEnabled() const
{
    return m_bFogEnabled;
}

void Environment::SetFogRadiance(const Vector3 _radiance)
{
    m_fogRadiance = _radiance;
    m_bDirty      = true;
}

Vector3 Environment::GetFogRadiance() const
{
    return m_fogRadiance;
}

void Environment::SetFogDensity(const float _density)
{
    m_fogDensity = _density;
    m_bDirty     = true;
}

float Environment::GetFogDensity() const
{
    return m_fogDensity;
}

void Environment::SetFogFallOffStart(const float _start)
{
    m_fogFallOffStart = _start;
    m_bDirty          = true;
}

float Environment::GetFogFallOffStart() const
{
    return m_fogFallOffStart;
}

void Environment::SetFogFallOffEnd(const float _end)
{
    m_fogFallOffEnd = _end;
    m_bDirty        = true;
}

float Environment::GetFogFallOffEnd() const
{
    return m_fogFallOffEnd;
}

void Environment::SetFogHeightAttenuation(const float _attenuation)
{
    m_fogHeightAttenuation = _attenuation;
    m_bDirty               = true;
}

float Environment::GetFogHeightAttenuation() const
{
    return m_fogHeightAttenuation;
}

void Environment::SetToneMappingEnabled(const bool _bEnabled)
{
    m_bToneMappingEnabled = _bEnabled;
    m_bDirty              = true;
}

bool Environment::IsToneMappingEnabled() const
{
    return m_bToneMappingEnabled;
}

void Environment::SetToneMappingType(const eToneMappingType _type)
{
    m_toneMappingType = _type;
    m_bDirty          = true;
}

eToneMappingType Environment::GetToneMappingType() const
{
    return m_toneMappingType;
}

void Environment::SetToneMappingExposure(const float _exposure)
{
    m_toneMappingExposure = _exposure;
    m_bDirty              = true;
}

float Environment::GetToneMappingExposure() const
{
    return m_toneMappingExposure;
}

void Environment::SetToneMappingGamma(const float _gamma)
{
    m_toneMappingGamma = _gamma;
    m_bDirty           = true;
}

float Environment::GetToneMappingGamma() const
{
    return m_toneMappingGamma;
}

void Environment::SetBloomEnabled(const bool _bEnabled)
{
    m_bBloomEnabled = _bEnabled;
    m_bDirty        = true;
}

bool Environment::IsBloomEnabled() const
{
    return m_bBloomEnabled;
}

void Environment::SetBloomBlurRadius(const float _radius)
{
    m_bloomBlurRadius = _radius;
    m_bDirty          = true;
}

float Environment::GetBloomBlurRadius() const
{
    return m_bloomBlurRadius;
}

void Environment::SetBloomStrength(const float _strength)
{
    m_bloomStrength = _strength;
    m_bDirty        = true;
}

float Environment::GetBloomStrength() const
{
    return m_bloomStrength;
}

void Environment::SetBloomNumBlurPasses(const Int32 _numPasses)
{
    m_bloomNumBlurPasses = Clamp(_numPasses, 0, k_maxNumbloomPasses);
    m_bDirty             = true;
}

Int32 Environment::GetBloomNumBlurPasses() const
{
    return m_bloomNumBlurPasses;
}

void Environment::SetFxaaEnabled(const bool _bEnabled)
{
    m_bFxaaEnabled = _bEnabled;
    m_bDirty       = true;
}

bool Environment::IsFxaaEnabled() const
{
    return m_bFxaaEnabled;
}

void Environment::SetFxaaQuality(const eFxaaQuality _quality)
{
    m_fxaaQuality = _quality;
    m_bDirty      = true;
}

eFxaaQuality Environment::GetFxaaQuality() const
{
    return m_fxaaQuality;
}

bool Environment::IsDirty() const
{
    return m_bDirty;
}

void Environment::ClearDirty()
{
    m_bDirty = false;
}

void Environment::WriteToArchive(
    ArchiveWriter& _writer) const
{
    _writer.WriteField("directionalLightEnabled", m_bDirLightEnabled);
    _writer.WriteField("directionalLightRadiance", m_dirLightRadiance);
    _writer.WriteField("directionalLightIntensity", m_dirLightIntensity);
    _writer.WriteField("directionalLightDirection", m_dirLightDirection);
    _writer.WriteField("directionalLightShadowEnabled", m_bDirLightShadowEnabled);
    _writer.WriteField("directionalLightShadowBias", m_dirLightShadowBias);
    _writer.WriteField("directionalLightCascadeDistances", m_dirLightCascadeDistances);
    _writer.WriteField("iblIrradianceTex", m_pIblIrradianceTex);
    _writer.WriteField("iblSpecularTex", m_pIblSpecularTex);
    _writer.WriteField("iblBrdfLutTex", m_pIblBrdfLutTex);
    _writer.WriteField("iblIrradianceIntensity", m_iblIrradianceTexIntensity);
    _writer.WriteField("iblSpecularIntensity", m_iblSpecularTexIntensity);
    _writer.WriteField("skyboxType", m_skyboxType);
    _writer.WriteField("skyboxTex", m_pSkyboxTex);
    _writer.WriteField("skyboxSolidColor", m_skyboxSolidColor);
    _writer.WriteField("ssaoEnabled", m_bSsaoEnabled);
    _writer.WriteField("ssaoRadius", m_ssaoRadius);
    _writer.WriteField("ssaoBias", m_ssaoBias);
    _writer.WriteField("fogEnabled", m_bFogEnabled);
    _writer.WriteField("fogRadiance", m_fogRadiance);
    _writer.WriteField("fogDensity", m_fogDensity);
    _writer.WriteField("fogFallOffStart", m_fogFallOffStart);
    _writer.WriteField("fogFallOffEnd", m_fogFallOffEnd);
    _writer.WriteField("fogHeightAttenuation", m_fogHeightAttenuation);
    _writer.WriteField("toneMappingEnabled", m_bToneMappingEnabled);
    _writer.WriteField("toneMappingType", m_toneMappingType);
    _writer.WriteField("toneMappingExposure", m_toneMappingExposure);
    _writer.WriteField("toneMappingGamma", m_toneMappingGamma);
    _writer.WriteField("bloomEnabled", m_bBloomEnabled);
    _writer.WriteField("bloomBlurRadius", m_bloomBlurRadius);
    _writer.WriteField("bloomStrength", m_bloomStrength);
    _writer.WriteField("bloomNumBlurPasses", m_bloomNumBlurPasses);
    _writer.WriteField("fxaaEnabled", m_bFxaaEnabled);
    _writer.WriteField("fxaaQuality", m_fxaaQuality);
}

Error Environment::ReadFromArchive(
    ArchiveReader& _reader)
{
    if (const Error err = _reader.ReadField("directionalLightEnabled", m_bDirLightEnabled))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("directionalLightRadiance", m_dirLightRadiance))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("directionalLightIntensity", m_dirLightIntensity))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("directionalLightDirection", m_dirLightDirection))   
    {
        return err;
    }
    if (const Error err = _reader.ReadField("directionalLightShadowEnabled", m_bDirLightShadowEnabled))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("directionalLightShadowBias", m_dirLightShadowBias))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("directionalLightCascadeDistances", m_dirLightCascadeDistances))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("iblIrradianceTex", m_pIblIrradianceTex))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("iblSpecularTex", m_pIblSpecularTex))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("iblBrdfLutTex", m_pIblBrdfLutTex))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("iblIrradianceIntensity", m_iblIrradianceTexIntensity))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("iblSpecularIntensity", m_iblSpecularTexIntensity))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("skyboxType", m_skyboxType))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("skyboxTex", m_pSkyboxTex))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("skyboxSolidColor", m_skyboxSolidColor))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("ssaoEnabled", m_bSsaoEnabled))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("ssaoRadius", m_ssaoRadius))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("ssaoBias", m_ssaoBias))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("fogEnabled", m_bFogEnabled))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("fogRadiance", m_fogRadiance))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("fogDensity", m_fogDensity))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("fogFallOffStart", m_fogFallOffStart))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("fogFallOffEnd", m_fogFallOffEnd))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("fogHeightAttenuation", m_fogHeightAttenuation))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("toneMappingEnabled", m_bToneMappingEnabled))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("toneMappingType", m_toneMappingType))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("toneMappingExposure", m_toneMappingExposure))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("toneMappingGamma", m_toneMappingGamma))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("bloomEnabled", m_bBloomEnabled))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("bloomBlurRadius", m_bloomBlurRadius))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("bloomStrength", m_bloomStrength))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("bloomNumBlurPasses", m_bloomNumBlurPasses))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("fxaaEnabled", m_bFxaaEnabled))
    {
        return err;
    }
    if (const Error err = _reader.ReadField("fxaaQuality", m_fxaaQuality))
    {
        return err;
    }

    m_bDirty = true;   

    return Error::None;
}

}   // namespace Jam