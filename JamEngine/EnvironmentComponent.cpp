#include "pch.h"

#include "EnvironmentComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

void EnvironmentComponent::Serialize(
    JsonWriter        _writer,
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    // Directional Light
    {
        const DirectionalLightData& dirLight = environment.GetDirectionalLight();

        JsonWriter dirLightWriter = _writer.EnterField("dirLight");
        dirLightWriter.WriteField("enabled", dirLight.bEnabled);
        dirLightWriter.WriteField("radiance", dirLight.radiance);
        dirLightWriter.WriteField("intensity", dirLight.intensity);
        dirLightWriter.WriteField("direction", dirLight.direction);
        dirLightWriter.WriteField("shadowEnabled", dirLight.bShadowEnabled);
        dirLightWriter.WriteField("cascadeDistances", dirLight.cascadeDistances);
    }

    // Image Based Lighting
    {
        const ImageBasedLightingData& ibl = environment.GetIBL();

        JsonWriter iblWriter = _writer.EnterField("ibl");
        iblWriter.WriteField("irradianceTexturePath", ibl.pIrradianceTexture);
        iblWriter.WriteField("specularMapPath", ibl.pSpecularTexture);
        iblWriter.WriteField("brdfLUTPath", ibl.pBrdfLutTexture);
        iblWriter.WriteField("irradianceTextureIntensity", ibl.irradianceTextureIntensity);
        iblWriter.WriteField("specularTextureIntensity", ibl.specularTextureIntensity);
    }

    // Skybox
    {
        const SkyboxData& skybox = environment.GetSkybox();

        JsonWriter skyboxWriter = _writer.EnterField("skybox");
        skyboxWriter.WriteField("type", skybox.type);
        skyboxWriter.WriteField("path", skybox.pSkyboxTexture);
        skyboxWriter.WriteField("solidColor", skybox.solidColor);
    }

    // SSAO
    {
        const SSAOData& ssao = environment.GetSSAO();

        JsonWriter ssaoWriter = _writer.EnterField("ssao");
        ssaoWriter.WriteField("enabled", ssao.bEnabled);
        ssaoWriter.WriteField("radius", ssao.radius);
        ssaoWriter.WriteField("bias", ssao.bias);
    }

    // Fog
    {
        const FogData& fog = environment.GetFog();

        JsonWriter fogWriter = _writer.EnterField("fog");
        fogWriter.WriteField("enabled", fog.bEnabled);
        fogWriter.WriteField("radiance", fog.radiance);
        fogWriter.WriteField("density", fog.density);
        fogWriter.WriteField("fallOffStart", fog.fallOffStart);
        fogWriter.WriteField("fallOffEnd", fog.fallOffEnd);
        fogWriter.WriteField("heightAttenuation", fog.heightAttenuation);
    }

    // Tone Mapping
    {
        const ToneMappingData& toneMapping = environment.GetToneMapping();

        JsonWriter toneMappingWriter = _writer.EnterField("toneMapping");
        toneMappingWriter.WriteField("enabled", toneMapping.bEnabled);
        toneMappingWriter.WriteField("type", toneMapping.type);
        toneMappingWriter.WriteField("exposure", toneMapping.exposure);
        toneMappingWriter.WriteField("gamma", toneMapping.gamma);
    }

    // Bloom
    {
        const BloomData& bloom = environment.GetBloom();

        JsonWriter bloomWriter = _writer.EnterField("bloom");
        bloomWriter.WriteField("enabled", bloom.bEnabled);
        bloomWriter.WriteField("blurRadius", bloom.blurRadius);
        bloomWriter.WriteField("bloomStrength", bloom.bloomStrength);
        bloomWriter.WriteField("numBlurPasses", bloom.numBlurPasses);
    }

    // FXAA
    {
        const FXAAData& fxaa = environment.GetFXAA();

        JsonWriter fxaaWriter = _writer.EnterField("fxaa");
        fxaaWriter.WriteField("enabled", fxaa.bEnabled);
        fxaaWriter.WriteField("quality", fxaa.quality);
    }

    // Misc
    {
        const EnvironmentMisc& misc = environment.GetMisc();

        JsonWriter miscWriter = _writer.EnterField("misc");
        miscWriter.WriteField("shadowBias", misc.shadowBias);
    }
}

void EnvironmentComponent::Deserialize(
    JsonReader        _reader,
    const Ref<Scene>& _pScene,
    Entity            _owner)
{
    // Directional Light
    JsonReader dirLightReader;
    if (MoveIfHasValue(_reader.EnterField("dirLight"), dirLightReader))
    {
        DirectionalLightData dirLight;
        dirLightReader.ReadField("enabled", dirLight.bEnabled);
        dirLightReader.ReadField("radiance", dirLight.radiance);
        dirLightReader.ReadField("intensity", dirLight.intensity);
        dirLightReader.ReadField("direction", dirLight.direction);
        dirLightReader.ReadField("shadowEnabled", dirLight.bShadowEnabled);
        dirLightReader.ReadField("cascadeDistances", dirLight.cascadeDistances);
        environment.SetDirectionalLight(dirLight);
    }

    // Image Based Lighting
    JsonReader iblReader;
    if (MoveIfHasValue(_reader.EnterField("ibl"), iblReader))
    {
        ImageBasedLightingData ibl;
        iblReader.ReadField("irradianceTexturePath", ibl.pIrradianceTexture);
        iblReader.ReadField("specularMapPath", ibl.pSpecularTexture);
        iblReader.ReadField("brdfLUTPath", ibl.pBrdfLutTexture);
        iblReader.ReadField("irradianceTextureIntensity", ibl.irradianceTextureIntensity);
        iblReader.ReadField("specularTextureIntensity", ibl.specularTextureIntensity);
        environment.SetIBL(ibl);
    }

    // Skybox
    JsonReader skyboxReader;
    if (MoveIfHasValue(_reader.EnterField("skybox"), skyboxReader))
    {
        SkyboxData skybox;
        skyboxReader.ReadField("type", skybox.type);
        skyboxReader.ReadField("path", skybox.pSkyboxTexture);
        skyboxReader.ReadField("solidColor", skybox.solidColor);
        environment.SetSkybox(skybox);
    }

    // SSAO
    JsonReader ssaoReader;
    if (MoveIfHasValue(_reader.EnterField("ssao"), ssaoReader))
    {
        SSAOData ssao;
        ssaoReader.ReadField("enabled", ssao.bEnabled);
        ssaoReader.ReadField("radius", ssao.radius);
        ssaoReader.ReadField("bias", ssao.bias);
        environment.SetSSAO(ssao);
    }

    // Fog
    JsonReader fogReader;
    if (MoveIfHasValue(_reader.EnterField("fog"), fogReader))
    {
        FogData fog;
        fogReader.ReadField("enabled", fog.bEnabled);
        fogReader.ReadField("radiance", fog.radiance);
        fogReader.ReadField("density", fog.density);
        fogReader.ReadField("fallOffStart", fog.fallOffStart);
        fogReader.ReadField("fallOffEnd", fog.fallOffEnd);
        fogReader.ReadField("heightAttenuation", fog.heightAttenuation);
        environment.SetFog(fog);
    }

    // Tone Mapping
    JsonReader toneMappingReader;
    if (MoveIfHasValue(_reader.EnterField("toneMapping"), toneMappingReader))
    {
        ToneMappingData toneMapping;
        toneMappingReader.ReadField("enabled", toneMapping.bEnabled);
        toneMappingReader.ReadField("type", toneMapping.type);
        toneMappingReader.ReadField("exposure", toneMapping.exposure);
        toneMappingReader.ReadField("gamma", toneMapping.gamma);
        environment.SetToneMapping(toneMapping);
    }

    // Bloom
    JsonReader bloomReader;
    if (MoveIfHasValue(_reader.EnterField("bloom"), bloomReader))
    {
        BloomData bloom;
        bloomReader.ReadField("enabled", bloom.bEnabled);
        bloomReader.ReadField("blurRadius", bloom.blurRadius);
        bloomReader.ReadField("bloomStrength", bloom.bloomStrength);
        bloomReader.ReadField("numBlurPasses", bloom.numBlurPasses);
        environment.SetBloom(bloom);
    }

    // FXAA
    JsonReader fxaaReader;
    if (MoveIfHasValue(_reader.EnterField("fxaa"), fxaaReader))
    {
        FXAAData fxaa;
        fxaaReader.ReadField("enabled", fxaa.bEnabled);
        fxaaReader.ReadField("quality", fxaa.quality);
        environment.SetFXAA(fxaa);
    }

    // Misc
    JsonReader miscReader;
    if (MoveIfHasValue(_reader.EnterField("misc"), miscReader))
    {
        EnvironmentMisc misc;
        miscReader.ReadField("shadowBias", misc.shadowBias);
        environment.SetMisc(misc);
    }
}

void EnvironmentComponent::Panel_EditComponent(
    Ref<Scene>& _pScene,
    Entity      _entity)
{
    ImGui::SeparatorText("directional light");
    if (BeginLabelValueTable("EnvironmentComponent"))
    {
        // use for direction editing
        static Vec2 s_pyRad      = {};
        static bool s_bPyEditing = false;

        DirectionalLightData dirLight = environment.GetDirectionalLight();
        bool                 bDirty   = false;

        // row: enabled
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##DirLightEnabled", &dirLight.bEnabled);
        }

        // row: radiance
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("radiance");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= ImGui::ColorEdit3("##DirLightRadiance", &dirLight.radiance.x);
        }

        // row: intensity
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("intensity");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##DirLightIntensity", &dirLight.intensity, 0.01f, 0.f);
        }

        // row: direction
        {
            if (!s_bPyEditing)
            {
                s_pyRad = ForwardToPitchYaw(dirLight.direction);
            }

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("direction");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            if (Widget_DragPitchYaw("##DirLightDirection", s_pyRad))
            {
                s_bPyEditing = true;
                bDirty       = true;
            }

            if (ImGui::IsItemDeactivated())
            {
                s_bPyEditing = false;
            }
        }

        // row: shadow cast
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("shadow cast");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##DirLightShadowCast", &dirLight.bShadowEnabled);
        }

        // cascade distances
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("cascade distances");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();

            bDirty |= Widget_DragFloat("##CascadeDistance1", &dirLight.cascadeDistances[0], 0.01f, 0.f, dirLight.cascadeDistances[1], "%.3fm");
            bDirty |= Widget_DragFloat("##CascadeDistance2", &dirLight.cascadeDistances[1], 0.01f, dirLight.cascadeDistances[0], dirLight.cascadeDistances[2], "%.3fm");
            bDirty |= Widget_DragFloat("##CascadeDistance3", &dirLight.cascadeDistances[2], 0.01f, dirLight.cascadeDistances[1], FLT_MAX, "%.3fm");
        }

        if (bDirty)
        {
            dirLight.direction = PitchYawToForward(s_pyRad.x, s_pyRad.y);
            environment.SetDirectionalLight(dirLight);
        }

        ImGui::EndTable()();
    }

    ImGui::SeparatorText("image based lighting");
    if (BeginLabelValueTable("EnvironmentComponent"))
    {
        ImageBasedLightingData ibl    = environment.GetIBL();
        bool                   bDirty = false;

        // row: irradiance texture
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("irradiance texture");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_ImageTextureSelector("##IBLIrradianceTexture", ibl.pIrradianceTexture);
        }

        // row: irradiance texture intensity
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("irradiance intensity");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##IBLIrradianceIntensity", &ibl.irradianceTextureIntensity, 0.01f, 0.f);
        }

        // row: specular texture
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("specular texture");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_ImageTextureSelector("##IBLSpecularTexture", ibl.pSpecularTexture);
        }

        // row: specular texture intensity
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("specular intensity");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##IBLSpecularIntensity", &ibl.specularTextureIntensity, 0.01f, 0.f);
        }

        // row: BRDF LUT texture
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("Brdf Lut texture");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_ImageTextureSelector("##IBLBrdfLUTTexture", ibl.pBrdfLutTexture);
        }

        if (bDirty)
        {
            environment.SetIBL(ibl);
        }

        ImGui::EndTable()();
    }

    ImGui::SeparatorText("skybox");
    if (BeginLabelValueTable("EnvironmentComponent"))
    {
        SkyboxData skybox = environment.GetSkybox();
        bool       bDirty = false;

        // row: type
        {
            constexpr const char* k_names[] = { "solid color", "cube map" };

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("type");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_Combo("##SkyboxType", reinterpret_cast<Int32&>(skybox.type), k_names);
        }

        // row: texture
        if (skybox.type == eSkyboxType::CubeMap)
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("texture");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_ImageTextureSelector("##SkyboxTexture", skybox.pSkyboxTexture);
        }

        // row: solid color
        if (skybox.type == eSkyboxType::SolidColor)
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("solid color");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= ImGui::ColorEdit3("##SkyboxSolidColor", &skybox.solidColor.x);
        }

        if (bDirty)
        {
            environment.SetSkybox(skybox);
        }

        ImGui::EndTable()();
    }

    // ssao
    ImGui::SeparatorText("ssao");
    if (BeginLabelValueTable("EnvironmentComponent"))
    {
        SSAOData ssao   = environment.GetSSAO();
        bool     bDirty = false;

        // row: enabled
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##SSAOEnabled", &ssao.bEnabled);
        }

        // row: radius
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("radius");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##SSAORadius", &ssao.radius, 0.001f, 0.f, FLT_MAX, "%.4f");
        }

        // row: bias
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("bias");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##SSAOBias", &ssao.bias, 0.001f, 0.f, FLT_MAX, "%.4f");
        }

        if (bDirty)
        {
            environment.SetSSAO(ssao);
        }

        ImGui::EndTable()();
    }

    ImGui::SeparatorText("height fog");
    if (BeginLabelValueTable("EnvironmentComponent"))
    {
        FogData fog    = environment.GetFog();
        bool    bDirty = false;

        // row: enabled
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##FogEnabled", &fog.bEnabled);
        }

        // row: radiance
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("radiance");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= ImGui::ColorEdit3("##FogRadiance", &fog.radiance.x);
        }

        // row: density
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("density");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##FogDensity", &fog.density, 0.0001f, 0.f, FLT_MAX, "%.5f");
        }

        // row: fall off start
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("fall off start");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##FogFallOffStart", &fog.fallOffStart, 0.01f, 0.f, fog.fallOffEnd, "%.3fm");
        }

        // row: fall off end
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("fall off end");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##FogFallOffEnd", &fog.fallOffEnd, 0.01f, fog.fallOffStart, FLT_MAX, "%.3fm");
        }

        // row: height attenuation
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("height attenuation");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##FogHeightAttenuation", &fog.heightAttenuation, 0.001f, 0.f, FLT_MAX, "%.3f");
        }

        if (bDirty)
        {
            environment.SetFog(fog);
        }

        ImGui::EndTable()();
    }

    // tonemap
    ImGui::SeparatorText("tone mapping");
    if (BeginLabelValueTable("EnvironmentComponent"))
    {
        ToneMappingData toneMapping = environment.GetToneMapping();
        bool            bDirty      = false;

        // row: enabled
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##ToneMappingEnabled", &toneMapping.bEnabled);
        }

        // row: type
        {
            constexpr const char* k_name[] = { "linear", "reinhard", "luma based reinhard", "white preserving reinhard", "rom bin da house", "uncharted2", "filmic" };

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("type");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_Combo("##ToneMappingType", reinterpret_cast<Int32&>(toneMapping.type), k_name);
        }

        // row: exposure
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("exposure");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##ToneMappingExposure", &toneMapping.exposure, 0.01f, 0.f, FLT_MAX, "%.3f");
        }

        // row: gamma
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("gamma");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##ToneMappingGamma", &toneMapping.gamma, 0.01f, 0.f, FLT_MAX, "%.3f");
        }

        if (bDirty)
        {
            environment.SetToneMapping(toneMapping);
        }

        ImGui::EndTable()();
    }

    // bloom
    ImGui::SeparatorText("bloom");
    if (BeginLabelValueTable("EnvironmentComponent"))
    {
        BloomData bloom  = environment.GetBloom();
        bool      bDirty = false;

        // row: enabled
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##BloomEnabled", &bloom.bEnabled);
        }

        // row: blur radius
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("blur radius");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            if (Widget_DragFloat("##BloomBlurRadius", &bloom.blurRadius, 0.001f))
            {
                bloom.blurRadius = Max(bloom.blurRadius, 0.f);
                bDirty |= true;
            }
        }

        // row: combine factor
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("combine factor");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##BloomCombineFactor", &bloom.bloomStrength, 0.01f, 0.f);
        }

        // row: number of blur passes
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("number of blur passes");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragInt("##BloomNumBlurPasses", &bloom.numBlurPasses, 0.05, 0, BloomData::k_maxBlurPasses);
        }

        if (bDirty)
        {
            environment.SetBloom(bloom);
        }

        ImGui::EndTable()();
    }

    // fxaa
    ImGui::SeparatorText("fxaa");
    if (BeginLabelValueTable("EnvironmentComponent"))
    {
        FXAAData fxaa   = environment.GetFXAA();
        bool     bDirty = false;

        // row: enabled
        {

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##FXAAEnabled", &fxaa.bEnabled))
            {
                bDirty = true;
            }
        }

        // row: quality
        {
            constexpr const char* k_qualityLevels[] = { "very low", "low", "medium", "high", "very high", "ultra" };

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("quality");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_Combo("##FXAAQuality", reinterpret_cast<Int32&>(fxaa.quality), k_qualityLevels);
        }

        if (bDirty)
        {
            environment.SetFXAA(fxaa);
        }

        ImGui::EndTable()();
    }

    // misc
    ImGui::SeparatorText("miscellaneous");
    if (BeginLabelValueTable("EnvironmentComponent"))
    {
        EnvironmentMisc misc   = environment.GetMisc();
        bool            bDirty = false;

        // row: shadow bias
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("shadow bias");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            bDirty |= Widget_DragFloat("##ShadowBias", &misc.shadowBias, 0.0001f, 0.f, FLT_MAX, "%.5f");
        }

        if (bDirty)
        {
            environment.SetMisc(misc);
        }

        ImGui::EndTable()();
    }
}

}   // namespace jam