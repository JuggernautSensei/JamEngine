#include "pch.h"

#include "EnvironmentComponent.h"

#include "AssetManager.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "JsonUtilities.h"

namespace jam
{

Json EnvironmentComponent::Serialize(const Ref<Scene>& _pScene, Entity _owner) const
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
        dirLightJson["cascadeLevel"]     = dirLight.cascadeLevel;
        dirLightJson["cascadeDistances"] = dirLight.cascadeDistances;

        json["dirLight"] = std::move(dirLightJson);
    }

    // Image Based Lighting
    {
        Json iblJson;
        iblJson["irradianceTexturePath"] = AssetToJson(ibl.pIrradianceTexture);
        iblJson["specularMapPath"]       = AssetToJson(ibl.pSpecularTexture);
        iblJson["brdfLUTPath"]           = AssetToJson(ibl.pBrdfLutTexture);

        iblJson["irradianceTextureIntensity"] = ibl.irradianceTextureIntensity;
        iblJson["specularTextureIntensity"]   = ibl.specularTextureIntensity;

        json["ibl"] = std::move(iblJson);
    }

    // Skybox
    {
        Json skyboxJson;

        skyboxJson["type"]       = skybox.type;
        skyboxJson["path"]       = AssetToJson(skybox.pSkyboxTexture);
        skyboxJson["solidColor"] = skybox.solidColor;

        json["skybox"] = std::move(skyboxJson);
    }

    // SSAO
    {
        Json ssaoJson;

        ssaoJson["enabled"] = ssao.bEnabled;
        ssaoJson["radius"]  = ssao.radius;
        ssaoJson["bias"]    = ssao.bias;

        json["ssao"] = std::move(ssaoJson);
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

        json["fog"] = std::move(fogJson);
    }

    // Tone Mapping
    {
        Json toneMappingJson;

        toneMappingJson["enabled"]  = toneMapping.bEnabled;
        toneMappingJson["type"]     = toneMapping.type;
        toneMappingJson["exposure"] = toneMapping.exposure;
        toneMappingJson["gamma"]    = toneMapping.gamma;

        json["toneMapping"] = std::move(toneMappingJson);
    }

    // Bloom
    {
        Json bloomJson;

        bloomJson["enabled"]       = bloom.bEnabled;
        bloomJson["blurRadius"]    = bloom.blurRadius;
        bloomJson["bloomStrength"] = bloom.bloomStrength;
        bloomJson["numBlurPasses"] = bloom.numBlurPasses;

        json["bloom"] = std::move(bloomJson);
    }

    // FXAA
    {
        Json fxaaJson;

        fxaaJson["enabled"] = fxaa.bEnabled;
        fxaaJson["quality"] = fxaa.quality;

        json["fxaa"] = std::move(fxaaJson);
    }

    // Misc
    {
        Json miscJson;

        miscJson["shadowBias"] = misc.shadowBias;

        json["misc"] = std::move(miscJson);
    }

    return json;
}

bool EnvironmentComponent::Deserialize(const Json& _json, Ref<Scene>& _pScene, Entity _owner)
{
    try
    {
        // Directional Light
        {
            const Json& json = _json.at("dirLight");

            DirectionalLightData dirLight;
            dirLight.bEnabled         = json.value("enabled", dirLight.bEnabled);
            dirLight.radiance         = json.value("radiance", dirLight.radiance);
            dirLight.intensity        = json.value("intensity", dirLight.intensity);
            dirLight.direction        = json.value("direction", dirLight.direction);
            dirLight.bShadowEnabled   = json.value("shadowEnabled", dirLight.bShadowEnabled);
            dirLight.cascadeLevel     = json.value("cascadeLevel", dirLight.cascadeLevel);
            dirLight.cascadeDistances = json.value("cascadeDistances", dirLight.cascadeDistances);

            environment.SetDirectionalLight(dirLight);
        }

        // Image Based Lighting
        {
            const Json&            json = _json.at("ibl");
            ImageBasedLightingData ibl;

            // Irradiance Texture
            {
                const Json& path       = json.at("irradianceTexturePath");
                ibl.pIrradianceTexture = AssetFromJsonOrThrow<ImageTexture>(path);
            }

            // Specular Texture
            {
                const Json& path     = json.at("specularMapPath");
                ibl.pSpecularTexture = AssetFromJsonOrThrow<ImageTexture>(path);
            }

            // BRDF LUT
            {
                const Json& path    = json.at("brdfLUTPath");
                ibl.pBrdfLutTexture = AssetFromJsonOrThrow<ImageTexture>(path);
            }

            ibl.irradianceTextureIntensity = json.value("irradianceTextureIntensity", ibl.irradianceTextureIntensity);
            ibl.specularTextureIntensity   = json.value("specularTextureIntensity", ibl.specularTextureIntensity);
            environment.SetIBL(ibl);
        }

        // Skybox
        {
            const Json& json = _json.at("skybox");
            const Json& path = json.at("path");

            SkyboxData skybox;
            skybox.type           = json.value("type", skybox.type);
            skybox.solidColor     = json.value("solidColor", skybox.solidColor);
            skybox.pSkyboxTexture = AssetFromJsonOrThrow<ImageTexture>(path);

            environment.SetSkybox(skybox);
        }

        // SSAO
        {
            const Json& json = _json.at("ssao");

            SSAOData ssao;
            ssao.bEnabled = json.value("enabled", ssao.bEnabled);
            ssao.radius   = json.value("radius", ssao.radius);
            ssao.bias     = json.value("bias", ssao.bias);

            environment.SetSSAO(ssao);
        }

        // Fog
        {
            const Json& json = _json.at("fog");

            FogData fog;
            fog.bEnabled          = json.value("enabled", fog.bEnabled);
            fog.radiance          = json.value("radiance", fog.radiance);
            fog.density           = json.value("density", fog.density);
            fog.fallOffStart      = json.value("fallOffStart", fog.fallOffStart);
            fog.fallOffEnd        = json.value("fallOffEnd", fog.fallOffEnd);
            fog.heightAttenuation = json.value("heightAttenuation", fog.heightAttenuation);

            environment.SetFog(fog);
        }

        // Tone Mapping
        {
            const Json& json = _json.at("toneMapping");

            ToneMappingData toneMapping;
            toneMapping.bEnabled = json.value("enabled", toneMapping.bEnabled);
            toneMapping.type     = json.value("type", toneMapping.type);
            toneMapping.exposure = json.value("exposure", toneMapping.exposure);
            toneMapping.gamma    = json.value("gamma", toneMapping.gamma);

            environment.SetToneMapping(toneMapping);
        }

        // Bloom
        {
            const Json& json = _json.at("bloom");

            BloomData bloom;
            bloom.bEnabled      = json.value("enabled", bloom.bEnabled);
            bloom.blurRadius    = json.value("blurRadius", bloom.blurRadius);
            bloom.bloomStrength = json.value("bloomStrength", bloom.bloomStrength);
            bloom.numBlurPasses = json.value("numBlurPasses", bloom.numBlurPasses);

            environment.SetBloom(bloom);
        }

        // FXAA
        {
            const Json& json = _json.at("fxaa");

            FXAAData fxaa;
            fxaa.bEnabled = json.value("enabled", fxaa.bEnabled);
            fxaa.quality  = json.value("quality", fxaa.quality);

            environment.SetFXAA(fxaa);
        }

        // Misc
        if (_json.contains("misc"))
        {
            const Json& json = _json.at("misc");

            EnvironmentMisc misc = environment.GetMisc();
            misc.shadowBias      = json.value("shadowBias", misc.shadowBias);

            environment.SetMisc(misc);
        }

        return true;
    }
    catch (const std::exception& e)
    {
        SetErrorString(JAM_PRETTY_FUNCTION " failed: {}", e.what());
        return false;
    }
}

void EnvironmentComponent::DrawEditor(Ref<Scene>& _pScene, Entity _entity)
{
    ImGui::SeparatorText("directional light");
    if (ImGui::BeginTable("EnvironmentTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        // use for direction editing
        static PitchYaw s_pyRad   = {};
        static bool     s_bEditPy = false;

        DirectionalLightData dirLight = environment.GetDirectionalLight();
        bool                 bDirty   = false;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // row: enabled
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("enabled");
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
            Widget_FramePaddingAlignedText("radiance");
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
            Widget_FramePaddingAlignedText("intensity");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##DirLightIntensity", &dirLight.intensity, 0.05f, 0.f, 10000.f))
            {
                bDirty = true;
            }
        }

        // row: direction
        {
            if (!s_bEditPy)
            {
                s_pyRad = ForwardToPitchYaw(dirLight.direction);
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("direction");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragPitchYaw("##DirLightDirection", reinterpret_cast<Vec2*>(&s_pyRad)))
            {
                s_bEditPy = true;
                bDirty    = true;
            }

            if (ImGui::IsItemDeactivated())
            {
                s_bEditPy = false;
            }
        }

        // row: shadow cast
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("shadow cast");
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##DirLightShadowCast", &dirLight.bShadowEnabled))
            {
                bDirty = true;
            }
        }

        // row: cascade level
        {
            constexpr const char* k_cascadeLevelNames[] = { "2", "3", "4" };
            const char*           preview               = k_cascadeLevelNames[EnumToInt(dirLight.cascadeLevel)];

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("cascade level");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::BeginCombo("##DirLightCascadeLevel", preview))
            {
                for (Int32 i = 0; i < EnumCount<eCascadeLevel>(); ++i)
                {
                    const bool bSelected = (preview == k_cascadeLevelNames[i]);
                    if (ImGui::Selectable(k_cascadeLevelNames[i], bSelected))
                    {
                        dirLight.cascadeLevel = static_cast<eCascadeLevel>(i);
                        bDirty                = true;
                    }

                    if (bSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

        // cascade distances
        {
            constexpr Int32 k_numDistances[] = { 2, 3, 4 };
            constexpr float k_min            = 0.1f;
            constexpr float k_max            = 10000.f;
            const Int32     numDistances     = k_numDistances[EnumToInt(dirLight.cascadeLevel)];

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("cascade distances");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragScalarN("##CascadeDistances", ImGuiDataType_Float, dirLight.cascadeDistances.data(), numDistances, 0.1f, &k_min, &k_max, "%.1f", 0))
            {
                // clamp distances to be in ascending order
                for (Int32 i = 1; i < numDistances; ++i)
                {
                    if (dirLight.cascadeDistances[i] < dirLight.cascadeDistances[i - 1])
                    {
                        dirLight.cascadeDistances[i] = dirLight.cascadeDistances[i - 1];
                    }
                }

                bDirty = true;
            }
        }

        if (bDirty)
        {
            dirLight.direction = PitchYawToForward(s_pyRad.pitch, s_pyRad.yaw);
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

        // rendering texture cube is not supported yet

        // row: irradiance texture
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("irradiance texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_ImageTextureSelector("##IBLIrradianceTexture", ibl.pIrradianceTexture, { 0.f, 0.f }, true))
            {
                bDirty = true;
            }
        }

        // row: irradiance texture intensity
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("irradiance intensity");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_DragFloat("##IBLIrradianceIntensity", &ibl.irradianceTextureIntensity, 0.01f, 0.f, 100.f))
            {
                bDirty = true;
            }
        }

        // row: specular texture
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("specular texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_ImageTextureSelector("##IBLSpecularTexture", ibl.pSpecularTexture, { 0.f, 0.f }, true))
            {
                bDirty = true;
            }
        }

        // row: specular texture intensity
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("specular intensity");
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
            Widget_FramePaddingAlignedText("BRDF LUT texture");
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
            constexpr const char* k_skyboxTypeNames[] = { "solid color", "cube map" };
            const char*           preview             = k_skyboxTypeNames[EnumToInt(skybox.type)];

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("type");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::BeginCombo("##SkyboxType", preview))
            {
                for (Int32 i = 0; i < EnumCount<eSkyboxType>(); ++i)
                {
                    const bool bSelected = (preview == k_skyboxTypeNames[i]);
                    if (ImGui::Selectable(k_skyboxTypeNames[i], bSelected))
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
            Widget_FramePaddingAlignedText("texture");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_ImageTextureSelector("##SkyboxTexture", skybox.pSkyboxTexture, { 0.f, 0.f }, true))
            {
                bDirty = true;
            }
        }

        // row: solid color
        if (skybox.type == eSkyboxType::SolidColor)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("solid color");
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
            Widget_FramePaddingAlignedText("enabled");
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
            Widget_FramePaddingAlignedText("radius");
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
            Widget_FramePaddingAlignedText("bias");
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
            Widget_FramePaddingAlignedText("enabled");
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
            Widget_FramePaddingAlignedText("radiance");
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
            Widget_FramePaddingAlignedText("density");
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
            Widget_FramePaddingAlignedText("fall off start");
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
            Widget_FramePaddingAlignedText("fall off end");
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
            Widget_FramePaddingAlignedText("height attenuation");
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
            Widget_FramePaddingAlignedText("enabled");
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##ToneMappingEnabled", &toneMapping.bEnabled))
            {
                bDirty = true;
            }
        }

        // row: type
        {
            constexpr const char* k_toneMappingTypeNames[] = { "linear", "reinhard", "luma based reinhard", "white preserving reinhard", "rom bin da house", "uncharted2", "filmic" };
            const char*           preview                  = k_toneMappingTypeNames[EnumToInt(toneMapping.type)];

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FramePaddingAlignedText("type");
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
            Widget_FramePaddingAlignedText("exposure");
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
            Widget_FramePaddingAlignedText("gamma");
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
            Widget_FramePaddingAlignedText("enabled");
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
            Widget_FramePaddingAlignedText("blur radius");
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
            Widget_FramePaddingAlignedText("combine factor");
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
            Widget_FramePaddingAlignedText("number of blur passes");
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
            Widget_FramePaddingAlignedText("enabled");
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
            Widget_FramePaddingAlignedText("quality");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::BeginCombo("##FXAAQuality", k_qualityLevels[EnumToInt(fxaa.quality)]))
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
            Widget_FramePaddingAlignedText("shadow bias");
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