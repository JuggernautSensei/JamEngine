# Environment Struct Refactor Guide

## New Structure
```cpp
struct DirectionalLightData { Vec3 direction, float intensity, Vec3 color, bool bShadowCastEnabled, eCascadeLevel cascadeLevel, float cascadeShadowRanges[4], float shadowBias }
struct ImageBasedLightingData { irradianceMap, specularMap, brdfLUT, irradianceMapIntensity, specularMapIntensity }
struct SkyboxData { eSkyboxType type, texture, Vec3 solidColor }
struct SSAOData { bool bEnabled, float radius, float bias }
struct FogData { Vec3 color, float density, fallOffStart, fallOffEnd, heightAttenuation }
struct ToneMappingData { eToneMappingType type, float exposure, float gamma }
struct BloomData { float blurRadius, float combineFactor }

struct Environment {
    DirectionalLightData dirLight;
    ImageBasedLightingData ibl;
    SkyboxData skybox;
    SSAOData ssao;
    FogData fog;
    ToneMappingData toneMapping;
    BloomData bloom;
}
```

## Changes Required

### OLD -> NEW Mappings:
- `env.dirLightDirection` -> `env.dirLight.direction`
- `env.dirLightIntensity` -> `env.dirLight.intensity`
- `env.dirLightColor` -> `env.dirLight.color`
- `env.bDirLightShadowEnabled` -> `env.dirLight.bShadowCastEnabled`
- `env.dirLightShadowCascadeLevel` -> `env.dirLight.cascadeLevel`
- `env.dirLightCascadeShadowRanges` -> `env.dirLight.cascadeShadowRanges`
- `env.dirLightShadowBias` -> `env.dirLight.shadowBias`
- `env.irradianceMap` -> `env.ibl.irradianceMap`
- `env.specularMap` -> `env.ibl.specularMap`
- `env.brdfLUT` -> `env.ibl.brdfLUT`
- `env.irradianceMapIntensity` -> `env.ibl.irradianceMapIntensity`
- `env.specularMapIntensity` -> `env.ibl.specularMapIntensity`
- `env.skyboxType` -> `env.skybox.type`
- `env.skyboxTexture` -> `env.skybox.texture`
- `env.skyboxSolidColor` -> `env.skybox.solidColor`
- `env.bSSAOEnabled` -> `env.ssao.bEnabled`
- `env.ssaoRadius` -> `env.ssao.radius`
- `env.ssaoBias` -> `env.ssao.bias`
- `env.fogColor` -> `env.fog.color`
- `env.fogDensity` -> `env.fog.density`
- `env.fogFallOffStart` -> `env.fog.fallOffStart`
- `env.fogFallOffEnd` -> `env.fog.fallOffEnd`
- `env.fogHeightAttenuation` -> `env.fog.heightAttenuation`
- `env.toneMappingType` -> `env.toneMapping.type`
- `env.exposure` -> `env.toneMapping.exposure`
- `env.gamma` -> `env.toneMapping.gamma`
- `env.bloomBlurRadius` -> `env.bloom.blurRadius`
- `env.bloomCombineFactor` -> `env.bloom.combineFactor`

## Files to Update:
1. JamEngine\BasicRenderer.cpp - UploadEnvironmentData() and UploadCascadeShadowCasterData()
2. JamEngine\EnvironmentComponent.cpp - Serialize(), Deserialize(), DrawEditor()
3. ExampleApp\HelloBasicRenderer.cpp - OnEnter() and CompileGraph_()
