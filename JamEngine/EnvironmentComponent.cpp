#include "pch.h"

#include "EnvironmentComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

void EnvironmentComponent::Serialize(
    JsonWriter&       _writer,
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
    JsonReader&       _reader,
    const Ref<Scene>& _pScene,
    Entity            _owner)
{

    // Directional Light
    if (Optional<JsonReader> dirLightReader = _reader.EnterField("dirLight"))
    {
        DirectionalLightData dirLight;
        dirLightReader->ReadField("enabled", dirLight.bEnabled);
        dirLightReader->ReadField("radiance", dirLight.radiance);
        dirLightReader->ReadField("intensity", dirLight.intensity);
        dirLightReader->ReadField("direction", dirLight.direction);
        dirLightReader->ReadField("shadowEnabled", dirLight.bShadowEnabled);
        dirLightReader->ReadField("cascadeDistances", dirLight.cascadeDistances);
        environment.SetDirectionalLight(dirLight);
    }

    // Image Based Lighting
    if (Optional<JsonReader> iblReader = _reader.EnterField("ibl"))
    {
        ImageBasedLightingData ibl;
        iblReader->ReadField("irradianceTexturePath", ibl.pIrradianceTexture);
        iblReader->ReadField("specularMapPath", ibl.pSpecularTexture);
        iblReader->ReadField("brdfLUTPath", ibl.pBrdfLutTexture);
        iblReader->ReadField("irradianceTextureIntensity", ibl.irradianceTextureIntensity);
        iblReader->ReadField("specularTextureIntensity", ibl.specularTextureIntensity);
        environment.SetIBL(ibl);
    }

    // Skybox
    if (Optional<JsonReader> skyboxReader = _reader.EnterField("skybox"))
    {
        SkyboxData skybox;
        skyboxReader->ReadField("type", skybox.type);
        skyboxReader->ReadField("path", skybox.pSkyboxTexture);
        skyboxReader->ReadField("solidColor", skybox.solidColor);
        environment.SetSkybox(skybox);
    }

    // SSAO
    if (Optional<JsonReader> ssaoReader = _reader.EnterField("ssao"))
    {
        SSAOData ssao;
        ssaoReader->ReadField("enabled", ssao.bEnabled);
        ssaoReader->ReadField("radius", ssao.radius);
        ssaoReader->ReadField("bias", ssao.bias);
        environment.SetSSAO(ssao);
    }

    // Fog
    if (Optional<JsonReader> fogReader = _reader.EnterField("fog"))
    {
        FogData fog;
        fogReader->ReadField("enabled", fog.bEnabled);
        fogReader->ReadField("radiance", fog.radiance);
        fogReader->ReadField("density", fog.density);
        fogReader->ReadField("fallOffStart", fog.fallOffStart);
        fogReader->ReadField("fallOffEnd", fog.fallOffEnd);
        fogReader->ReadField("heightAttenuation", fog.heightAttenuation);
        environment.SetFog(fog);
    }

    // Tone Mapping
    if (Optional<JsonReader> toneMappingReader = _reader.EnterField("toneMapping"))
    {
        ToneMappingData toneMapping;
        toneMappingReader->ReadField("enabled", toneMapping.bEnabled);
        toneMappingReader->ReadField("type", toneMapping.type);
        toneMappingReader->ReadField("exposure", toneMapping.exposure);
        toneMappingReader->ReadField("gamma", toneMapping.gamma);
        environment.SetToneMapping(toneMapping);
    }

    // Bloom
    if (Optional<JsonReader> bloomReader = _reader.EnterField("bloom"))
    {
        BloomData bloom;
        bloomReader->ReadField("enabled", bloom.bEnabled);
        bloomReader->ReadField("blurRadius", bloom.blurRadius);
        bloomReader->ReadField("bloomStrength", bloom.bloomStrength);
        bloomReader->ReadField("numBlurPasses", bloom.numBlurPasses);
        environment.SetBloom(bloom);
    }

    // FXAA
    if (Optional<JsonReader> fxaaReader = _reader.EnterField("fxaa"))
    {
        FXAAData fxaa;
        fxaaReader->ReadField("enabled", fxaa.bEnabled);
        fxaaReader->ReadField("quality", fxaa.quality);
        environment.SetFXAA(fxaa);
    }

    // Misc
    if (Optional<JsonReader> miscReader = _reader.EnterField("misc"))
    {
        EnvironmentMisc misc;
        miscReader->ReadField("shadowBias", misc.shadowBias);
        environment.SetMisc(misc);
    }
}

void EnvironmentComponent::DrawEditor(
    Ref<Scene>& _pScene,
    Entity      _entity)
{
    ImGui::SeparatorText("directional light");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // use for direction editing
        static Vec2 s_pyRad      = {};
        static bool s_bPyEditing = false;

        DirectionalLightData dirLight = environment.GetDirectionalLight();
        bool                 bDirty   = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: enabled
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##DirLightEnabled", &dirLight.bEnabled);
        }

        // row: radiance
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("radiance");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= ImGui::ColorEdit3("##DirLightRadiance", &dirLight.radiance.x);
        }

        // row: intensity
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("intensity");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_DragLightIntensity("##DirLightIntensity", dirLight.intensity);
        }

        // row: direction
        {
            if (!s_bPyEditing)
            {
                s_pyRad = ForwardToPitchYaw(dirLight.direction);
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("direction");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
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
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("shadow cast");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##DirLightShadowCast", &dirLight.bShadowEnabled);
        }

        // cascade distances
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("cascade distances");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();

            Vec3 distances = { dirLight.cascadeDistances[0], dirLight.cascadeDistances[1], dirLight.cascadeDistances[2] };
            if (Widget_DragDistance3("##CascadeDistances", distances, 0.01f, Camera::k_minZ, Camera::k_maxZ))
            {
                // ensure ascending order
                std::sort(&distances.x, &distances.x + 3);
                dirLight.cascadeDistances[0] = distances.x;
                dirLight.cascadeDistances[1] = distances.y;
                dirLight.cascadeDistances[2] = distances.z;
                bDirty |= true;
            }
        }

        if (bDirty)
        {
            dirLight.direction = PitchYawToForward(s_pyRad.x, s_pyRad.y);
            environment.SetDirectionalLight(dirLight);
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("image based lighting");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        ImageBasedLightingData ibl    = environment.GetIBL();
        bool                   bDirty = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: irradiance texture
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("irradiance texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_ImageTextureSelector("##IBLIrradianceTexture", ibl.pIrradianceTexture);
        }

        // row: irradiance texture intensity
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("irradiance intensity");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_DragLightIntensity("##IBLIrradianceIntensity", ibl.irradianceTextureIntensity);
        }

        // row: specular texture
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("specular texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_ImageTextureSelector("##IBLSpecularTexture", ibl.pSpecularTexture);
        }

        // row: specular texture intensity
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("specular intensity");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_DragLightIntensity("##IBLSpecularIntensity", ibl.specularTextureIntensity);
        }

        // row: BRDF LUT texture
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("Brdf Lut texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_ImageTextureSelector("##IBLBrdfLUTTexture", ibl.pBrdfLutTexture);
        }

        if (bDirty)
        {
            environment.SetIBL(ibl);
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("skybox");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        SkyboxData skybox = environment.GetSkybox();
        bool       bDirty = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: type
        {
            constexpr const char* k_names[] = { "solid color", "cube map" };

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("type");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_EnumCombo("##SkyboxType", reinterpret_cast<Int32&>(skybox.type), k_names);
        }

        // row: texture
        if (skybox.type == eSkyboxType::CubeMap)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_ImageTextureSelector("##SkyboxTexture", skybox.pSkyboxTexture);
        }

        // row: solid color
        if (skybox.type == eSkyboxType::SolidColor)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("solid color");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= ImGui::ColorEdit3("##SkyboxSolidColor", &skybox.solidColor.x);
        }

        if (bDirty)
        {
            environment.SetSkybox(skybox);
        }

        ImGui::EndTable();
    }

    // ssao
    ImGui::SeparatorText("ssao");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        SSAOData ssao   = environment.GetSSAO();
        bool     bDirty = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: enabled
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##SSAOEnabled", &ssao.bEnabled);
        }

        // row: radius
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("radius");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_DragFloat("##SSAORadius", &ssao.radius, 0.001f, 0.f, 10.f);
        }

        // row: bias
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("bias");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_DragFloat("##SSAOBias", &ssao.bias, 0.001f, 0.f, 10.f);
        }

        if (bDirty)
        {
            environment.SetSSAO(ssao);
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("height fog");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        FogData fog    = environment.GetFog();
        bool    bDirty = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: enabled
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty |= ImGui::Checkbox("##FogEnabled", &fog.bEnabled);
        }

        // row: radiance
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("radiance");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= ImGui::ColorEdit3("##FogRadiance", &fog.radiance.x);
        }

        // row: density
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("density");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_DragFloat("##FogDensity", &fog.density, 0.0001f, 0.f, 1.f, "%.4f");
        }

        // row: fall off start
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("fall off start");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_DragDistance("##FogFallOffStart", fog.fallOffStart, 0.01f, 0.f, fog.fallOffEnd);
        }

        // row: fall off end
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("fall off end");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_DragDistance("##FogFallOffEnd", fog.fallOffEnd, 0.01f, fog.fallOffStart, Camera::k_maxZ);
        }

        // row: height attenuation
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("height attenuation");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty |= Widget_DragFloat("##FogHeightAttenuation", &fog.heightAttenuation, 0.01f, 0.f, 1.f);
        }

        if (bDirty)
        {
            environment.SetFog(fog);
        }

        ImGui::EndTable();
    }

    // tonemap
    ImGui::SeparatorText("tone mapping");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        ToneMappingData toneMapping = environment.GetToneMapping();
        bool            bDirty      = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: enabled
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty &= ImGui::Checkbox("##ToneMappingEnabled", &toneMapping.bEnabled);
        }

        // row: type
        {
            constexpr const char* k_toneMappingTypeNames[] = { "linear", "reinhard", "luma based reinhard", "white preserving reinhard", "rom bin da house", "uncharted2", "filmic" };

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("type");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty &= Widget_EnumCombo("##ToneMappingType", reinterpret_cast<Int32&>(toneMapping.type), k_toneMappingTypeNames);
        }

        // row: exposure
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("exposure");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty &= Widget_DragFloat("##ToneMappingExposure", &toneMapping.exposure, 0.01f, 0.f, 100.f);
        }

        // row: gamma
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("gamma");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty &= Widget_DragFloat("##ToneMappingGamma", &toneMapping.gamma, 0.01f, 0.1f, 5.f);
        }

        if (bDirty)
        {
            environment.SetToneMapping(toneMapping);
        }

        ImGui::EndTable();
    }

    // bloom
    ImGui::SeparatorText("bloom");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        BloomData bloom  = environment.GetBloom();
        bool      bDirty = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: enabled
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            bDirty &= ImGui::Checkbox("##BloomEnabled", &bloom.bEnabled);
        }

        // row: blur radius
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("blur radius");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty &= Widget_DragFloat("##BloomBlurRadius", &bloom.blurRadius, 0.001f, 0.001f, 1.f);
        }

        // row: combine factor
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("combine factor");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty &= Widget_DragFloat("##BloomCombineFactor", &bloom.bloomStrength, 0.01f, 0.f, 1.f);
        }

        // row: number of blur passes
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("number of blur passes");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::InputInt("##BloomNumBlurPasses", &bloom.numBlurPasses))
            {
                bloom.numBlurPasses = Clamp(bloom.numBlurPasses, 1, BloomData::k_maxBlurPasses);
                bDirty              = true;
            }
        }

        if (bDirty)
        {
            environment.SetBloom(bloom);
        }

        ImGui::EndTable();
    }

    // fxaa
    ImGui::SeparatorText("fxaa");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        FXAAData fxaa   = environment.GetFXAA();
        bool     bDirty = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: enabled
        {
            ImGui::TableNextRow();
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

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("quality");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            Widget_EnumCombo("##FXAAQuality", reinterpret_cast<Int32&>(fxaa.quality), k_qualityLevels);
        }

        if (bDirty)
        {
            environment.SetFXAA(fxaa);
        }

        ImGui::EndTable();
    }

    // misc
    ImGui::SeparatorText("miscellaneous");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        EnvironmentMisc misc   = environment.GetMisc();
        bool            bDirty = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: shadow bias
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("shadow bias");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            bDirty &= Widget_DragFloat("##MiscShadowBias", &misc.shadowBias, 0.0001f, 0.f, 1.f);
        }

        if (bDirty)
        {
            environment.SetMisc(misc);
        }

        ImGui::EndTable();
    }
}

}   // namespace jam