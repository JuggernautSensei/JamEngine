#include "pch.h"

#include "EnvironmentComponent.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

// todo: texture2d cube. array imgui 포팅하기
// repactory..

namespace jam
{

Json EnvironmentComponent::Serialize(
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    const DirectionalLightData&   dirLight    = environment.GetDirectionalLight();
    const ImageBasedLightingData& ibl         = environment.GetIBL();
    const SkyboxData&             skybox      = environment.GetSkybox();
    const SSAOData&               ssao        = environment.GetSSAO();
    const FogData&                fog         = environment.GetFog();
    const ToneMappingData&        toneMapping = environment.GetToneMapping();
    const BloomData&              bloom       = environment.GetBloom();
    const FXAAData&               fxaa        = environment.GetFXAA();
    const EnvironmentMisc&        misc        = environment.GetMisc();

    Json json;

    // Directional Light
    {
        Json dirLightJson;
        dirLightJson["enabled"]          = dirLight.bShadowEnabled;
        dirLightJson["radiance"]         = dirLight.radiance;
        dirLightJson["intensity"]        = dirLight.intensity;
        dirLightJson["direction"]        = dirLight.direction;
        dirLightJson["shadowEnabled"]    = dirLight.bShadowEnabled;
        dirLightJson["cascadeDistances"] = dirLight.cascadeDistances;
        json["dirLight"]                 = std::move(dirLightJson);
    }

    // Image Based Lighting
    {
        Json iblJson;
        iblJson["irradianceTexturePath"]      = ibl.pIrradianceTexture;
        iblJson["specularMapPath"]            = ibl.pSpecularTexture;
        iblJson["brdfLutPath"]                = ibl.pBrdfLutTexture;
        iblJson["irradianceTextureIntensity"] = ibl.irradianceTextureIntensity;
        iblJson["specularTextureIntensity"]   = ibl.specularTextureIntensity;
        json["ibl"]                           = std::move(iblJson);
    }

    // Skybox
    {
        Json skyboxJson;
        skyboxJson["type"]       = skybox.type;
        skyboxJson["path"]       = skybox.pSkyboxTexture;
        skyboxJson["solidColor"] = skybox.solidColor;
        json["skybox"]           = std::move(skyboxJson);
    }

    // SSAO
    {
        Json ssaoJson;
        ssaoJson["enabled"] = ssao.bEnabled;
        ssaoJson["radius"]  = ssao.radius;
        ssaoJson["bias"]    = ssao.bias;
        json["ssao"]        = std::move(ssaoJson);
    }

    // Fog
    {
        Json fogJson;
        fogJson["enabled"]           = fog.bEnabled;
        fogJson["radiance"]          = fog.radiance;
        fogJson["density"]           = fog.density;
        fogJson["fallOffStart"]      = fog.fallOffStart;
        fogJson["fallOffEnd"]        = fog.fallOffEnd;
        fogJson["heightAttenuation"] = fog.heightAttenuation;
        json["fog"]                  = std::move(fogJson);
    }

    // Tone Mapping
    {
        Json toneMappingJson;
        toneMappingJson["enabled"]  = toneMapping.bEnabled;
        toneMappingJson["type"]     = toneMapping.type;
        toneMappingJson["exposure"] = toneMapping.exposure;
        toneMappingJson["gamma"]    = toneMapping.gamma;
        json["toneMapping"]         = std::move(toneMappingJson);
    }

    // Bloom
    {
        Json bloomJson;
        bloomJson["enabled"]       = bloom.bEnabled;
        bloomJson["blurRadius"]    = bloom.blurRadius;
        bloomJson["bloomStrength"] = bloom.bloomStrength;
        bloomJson["numBlurPasses"] = bloom.numBlurPasses;
        json["bloom"]              = std::move(bloomJson);
    }

    // FXAA
    {
        Json fxaaJson;
        fxaaJson["enabled"] = fxaa.bEnabled;
        fxaaJson["quality"] = fxaa.quality;
        json["fxaa"]        = std::move(fxaaJson);
    }

    // Misc
    {
        Json miscJson;
        miscJson["shadowBias"] = misc.shadowBias;
        json["misc"]           = std::move(miscJson);
    }

    return json;
}

void EnvironmentComponent::Deserialize(
    const Json& _json,
    Ref<Scene>& _pScene,
    Entity      _owner)
{
    // Directional Light
    if (const std::optional<Json> opt = FindJsonField(_json, "skybox"))
    {
        const Json& json = *opt;

        DirectionalLightData dirLight;
        TryParseJsonField(json, "enabled", dirLight.bEnabled);
        TryParseJsonField(json, "radiance", dirLight.radiance);
        TryParseJsonField(json, "intensity", dirLight.intensity);
        TryParseJsonField(json, "direction", dirLight.direction);
        TryParseJsonField(json, "shadowEnabled", dirLight.bShadowEnabled);
        TryParseJsonField(json, "cascadeDistances", dirLight.cascadeDistances);
        environment.SetDirectionalLight(dirLight);
    }

    // Image Based Lighting
    if (const std::optional<Json> opt = FindJsonField(_json, "ibl"))
    {
        const Json& json = *opt;

        ImageBasedLightingData ibl;
        TryParseJsonField(json, "irradianceTexturePath", ibl.pIrradianceTexture);
        TryParseJsonField(json, "specularMapPath", ibl.pSpecularTexture);
        TryParseJsonField(json, "brdfLUTPath", ibl.pBrdfLutTexture);
        TryParseJsonField(json, "irradianceTextureIntensity", ibl.irradianceTextureIntensity);
        TryParseJsonField(json, "specularTextureIntensity", ibl.specularTextureIntensity);
        environment.SetIBL(ibl);
    }

    // Skybox
    if (const std::optional<Json> opt = FindJsonField(_json, "skybox"))
    {
        const Json& json = *opt;

        SkyboxData skybox;
        TryParseJsonField(json, "type", skybox.type);
        TryParseJsonField(json, "solidColor", skybox.solidColor);
        TryParseJsonField(json, "path", skybox.pSkyboxTexture);
        environment.SetSkybox(skybox);
    }

    // SSAO
    if (const std::optional<Json> opt = FindJsonField(_json, "ssao"))
    {
        const Json& json = *opt;

        SSAOData ssao;
        TryParseJsonField(json, "enabled", ssao.bEnabled);
        TryParseJsonField(json, "radius", ssao.radius);
        TryParseJsonField(json, "bias", ssao.bias);
        environment.SetSSAO(ssao);
    }

    // Fog
    if (const std::optional<Json> opt = FindJsonField(_json, "fog"))
    {
        const Json& json = *opt;

        FogData fog;
        TryParseJsonField(json, "enabled", fog.bEnabled);
        TryParseJsonField(json, "radiance", fog.radiance);
        TryParseJsonField(json, "density", fog.density);
        TryParseJsonField(json, "fallOffStart", fog.fallOffStart);
        TryParseJsonField(json, "fallOffEnd", fog.fallOffEnd);
        TryParseJsonField(json, "heightAttenuation", fog.heightAttenuation);
        environment.SetFog(fog);
    }

    // Tone Mapping
    if (const std::optional<Json> opt = FindJsonField(_json, "toneMapping"))
    {
        const Json& json = *opt;

        ToneMappingData toneMapping;
        TryParseJsonField(json, "enabled", toneMapping.bEnabled);
        TryParseJsonField(json, "type", toneMapping.type);
        TryParseJsonField(json, "exposure", toneMapping.exposure);
        TryParseJsonField(json, "gamma", toneMapping.gamma);
        environment.SetToneMapping(toneMapping);
    }

    // Bloom
    if (const std::optional<Json> opt = FindJsonField(_json, "bloom"))
    {
        const Json& json = *opt;

        BloomData bloom;
        TryParseJsonField(json, "enabled", bloom.bEnabled);
        TryParseJsonField(json, "blurRadius", bloom.blurRadius);
        TryParseJsonField(json, "bloomStrength", bloom.bloomStrength);
        TryParseJsonField(json, "numBlurPasses", bloom.numBlurPasses);
        environment.SetBloom(bloom);
    }

    // FXAA
    if (const std::optional<Json> opt = FindJsonField(_json, "fxaa"))
    {
        const Json& json = *opt;

        FXAAData fxaa;
        TryParseJsonField(json, "enabled", fxaa.bEnabled);
        TryParseJsonField(json, "quality", fxaa.quality);
        environment.SetFXAA(fxaa);
    }

    // Misc
    if (const std::optional<Json> opt = FindJsonField(_json, "misc"))
    {
        const Json& json = *opt;

        EnvironmentMisc misc;
        TryParseJsonField(json, "shadowBias", misc.shadowBias);
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
        static Vec2          s_pyRad      = {};
        static bool          s_bPyEditing = false;
        DirectionalLightData dirLight     = environment.GetDirectionalLight();
        bool                 bDirty       = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: enabled
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("enabled");
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##DirLightEnabled", &dirLight.bEnabled))
            {
                bDirty = true;
            }
        }

        // row: radiance
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("radiance");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::ColorEdit3("##DirLightRadiance", &dirLight.radiance.x))
            {
                bDirty = true;
            }
        }

        // row: intensity
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("intensity");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##DirLightIntensity", &dirLight.intensity, 0.001f, 0.f, 10.f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            {
                bDirty = true;
            }
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
            if (ImGui::Checkbox("##DirLightShadowCast", &dirLight.bShadowEnabled))
            {
                bDirty = true;
            }
        }

        // cascade distances
        {
            const ImGuiStyle& style   = ImGui::GetStyle();
            const float       spacing = style.ItemInnerSpacing.x;

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("cascade distances");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();

            // 1
            if (Widget_DragFloat("##CascadeDistances1", &dirLight.cascadeDistances[0], 0.1f, k_cameraNearestZ, dirLight.cascadeDistances[1] - k_epsilon, "%.1f", ImGuiSliderFlags_AlwaysClamp))
            {
                bDirty = true;
            }

            // 2
            ImGui::SameLine(spacing);
            if (Widget_DragFloat("##CascadeDistances2", &dirLight.cascadeDistances[1], 0.1f, dirLight.cascadeDistances[0] + k_epsilon, dirLight.cascadeDistances[2] - k_epsilon, "%.1f", ImGuiSliderFlags_AlwaysClamp))
            {
                bDirty = true;
            }

            // 3
            ImGui::SameLine(spacing);
            if (Widget_DragFloat("##CascadeDistances3", &dirLight.cascadeDistances[2], 0.1f, dirLight.cascadeDistances[1] + k_epsilon, k_cameraFarestZ, "%.1f", ImGuiSliderFlags_AlwaysClamp))
            {
                bDirty = true;
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
            if (Widget_ImageTextureSelector("##IBLIrradianceTexture", ibl.pIrradianceTexture))
            {
                bDirty = true;
            }
        }

        // row: irradiance texture intensity
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("irradiance intensity");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##IBLIrradianceIntensity", &ibl.irradianceTextureIntensity, 0.001f, 0.f, 10.f))
            {
                bDirty = true;
            }
        }

        // row: specular texture
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("specular texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_ImageTextureSelector("##IBLSpecularTexture", ibl.pSpecularTexture, { 0.f, 0.f }))
            {
                bDirty = true;
            }
        }

        // row: specular texture intensity
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("specular intensity");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##IBLSpecularIntensity", &ibl.specularTextureIntensity, 0.01f, 0.f, 100.f))
            {
                bDirty = true;
            }
        }

        // row: BRDF LUT texture
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("Brdf Lut texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_ImageTextureSelector("##IBLBrdfLUTTexture", ibl.pBrdfLutTexture))
            {
                bDirty = true;
            }
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
            const char*           preview   = k_names[IntFrom(skybox.type)];

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("type");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::BeginCombo("##SkyboxType", preview))
            {
                for (Int32 i = 0; i < EnumCount<eSkyboxType>(); ++i)
                {
                    const bool bSelected = (preview == k_names[i]);
                    if (ImGui::Selectable(k_names[i], bSelected))
                    {
                        skybox.type = static_cast<eSkyboxType>(i);
                        bDirty      = true;
                    }

                    if (bSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

        // row: texture
        if (skybox.type == eSkyboxType::CubeMap)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_ImageTextureSelector("##SkyboxTexture", skybox.pSkyboxTexture, { 0.f, 0.f }))
            {
                bDirty = true;
            }
        }

        // row: solid color
        if (skybox.type == eSkyboxType::SolidColor)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("solid color");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::ColorEdit3("##SkyboxSolidColor", &skybox.solidColor.x))
            {
                bDirty = true;
            }
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
            if (ImGui::Checkbox("##SSAOEnabled", &ssao.bEnabled))
            {
                bDirty = true;
            }
        }

        // row: radius
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("radius");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##SSAORadius", &ssao.radius, 0.01f, 0.f, 10.f))
            {
                bDirty = true;
            }
        }

        // row: bias
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("bias");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##SSAOBias", &ssao.bias, 0.01f, 0.f, 100.f))
            {
                bDirty = true;
            }
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
            if (ImGui::Checkbox("##FogEnabled", &fog.bEnabled))
            {
                bDirty = true;
            }
        }

        // row: radiance
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("radiance");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::ColorEdit3("##FogRadiance", &fog.radiance.x))
            {
                bDirty = true;
            }
        }

        // row: density
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("density");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##FogDensity", &fog.density, 0.0001f, 0.f, 1.f))
            {
                bDirty = true;
            }
        }

        // row: fall off start
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("fall off start");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##FogFallOffStart", &fog.fallOffStart, 0.1f, 0.f, 10000.f))
            {
                fog.fallOffStart = Min(fog.fallOffStart, fog.fallOffEnd);
                bDirty           = true;
            }
        }

        // row: fall off end
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("fall off end");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##FogFallOffEnd", &fog.fallOffEnd, 0.1f, 0.f, 10000.f))
            {
                fog.fallOffEnd = Max(fog.fallOffEnd, fog.fallOffStart);
                bDirty         = true;
            }
        }

        // row: height attenuation
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("height attenuation");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##FogHeightAttenuation", &fog.heightAttenuation, 0.01f, 0.f, 1.f))
            {
                bDirty = true;
            }
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
            if (ImGui::Checkbox("##ToneMappingEnabled", &toneMapping.bEnabled))
            {
                bDirty = true;
            }
        }

        // row: type
        {
            constexpr const char* k_toneMappingTypeNames[] = { "linear", "reinhard", "luma based reinhard", "white preserving reinhard", "rom bin da house", "uncharted2", "filmic" };
            const char*           preview                  = k_toneMappingTypeNames[IntFrom(toneMapping.type)];

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("type");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::BeginCombo("##ToneMappingType", preview))
            {
                for (Int32 i = 0; i < EnumCount<eToneMappingType>(); ++i)
                {
                    const bool bSelected = (preview == k_toneMappingTypeNames[i]);
                    if (ImGui::Selectable(k_toneMappingTypeNames[i], bSelected))
                    {
                        toneMapping.type = static_cast<eToneMappingType>(i);
                        bDirty           = true;
                    }

                    if (bSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

        // row: exposure
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("exposure");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##ToneMappingExposure", &toneMapping.exposure, 0.01f, 0.f, 100.f))
            {
                bDirty = true;
            }
        }

        // row: gamma
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("gamma");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##ToneMappingGamma", &toneMapping.gamma, 0.01f, 0.1f, 5.f))
            {
                bDirty = true;
            }
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
            if (ImGui::Checkbox("##BloomEnabled", &bloom.bEnabled))
            {
                bDirty = true;
            }
        }

        // row: blur radius
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("blur radius");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##BloomBlurRadius", &bloom.blurRadius, 0.001f, 0.001f, 1.f))
            {
                bDirty = true;
            }
        }

        // row: combine factor
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("combine factor");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##BloomCombineFactor", &bloom.bloomStrength, 0.01f, 0.f, 1.f))
            {
                bDirty = true;
            }
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
                bloom.numBlurPasses = Clamp(bloom.numBlurPasses, 1, 10);
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
            if (ImGui::BeginCombo("##FXAAQuality", k_qualityLevels[IntFrom(fxaa.quality)]))
            {
                for (Int32 i = 0; i < EnumCount<eFxaaQuality>(); ++i)
                {
                    const bool bSelected = (fxaa.quality == static_cast<eFxaaQuality>(i));
                    if (ImGui::Selectable(k_qualityLevels[i], bSelected))
                    {
                        fxaa.quality = static_cast<eFxaaQuality>(i);
                        bDirty       = true;
                    }
                    if (bSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
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
            if (Widget_DragFloat("##MiscShadowBias", &misc.shadowBias, 0.0001f, 0.f, 1.f))
            {
                bDirty = true;
            }
        }

        if (bDirty)
        {
            environment.SetMisc(misc);
        }

        ImGui::EndTable();
    }
}

}   // namespace jam