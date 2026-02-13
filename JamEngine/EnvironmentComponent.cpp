#include "pch.h"

#include "EnvironmentComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

void EnvironmentComponent::Serialize(
    ArchiveWriter& _writer,
    const Ref<Scene>&,
    Entity) const
{
    _writer.Write(environment);
}

void EnvironmentComponent::Deserialize(
    ArchiveReader& _reader,
    const Ref<Scene>&,
    Entity)
{
    _reader.Read(environment);
}

void EnvironmentComponent::DrawEditPanel(
    const Ref<Scene>&,
    Entity)
{
    ImGui::SeparatorText("directional light");
    if (BeginLabelTable("EnvironmentComponent_DirLight"))
    {
        // row: enabled
        {
            bool bEnabled = environment.IsDirectionalLightEnabled();

            ImGui::TableNextColumn();
            Widget_Text("enabled", true);
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##DirLightEnabled", &bEnabled))
            {
                environment.SetDirectionalLightEnabled(bEnabled);
            }
        }

        // row: radiance
        {
            Vec3 radiance = environment.GetDirectionalLightRadiance();

            ImGui::TableNextColumn();
            Widget_Text("radiance", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (ImGui::ColorEdit3("##DirLightRadiance", &radiance.x))
            {
                environment.SetDirectionalLightRadiance(radiance);
            }
        }

        // row: intensity
        {
            float intensity = environment.GetDirectionalLightIntensity();

            ImGui::TableNextColumn();
            Widget_Text("intensity", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##DirLightIntensity",intensity, 0.01f, 0.f, Limits<float>::k_max))
            {
                environment.SetDirectionalLightIntensity(intensity);
            }
        }

        // row: direction
        {
            static EulerProxy s_proxy = {};

            Vec3 direction = environment.GetDirectionalLightDirection();
            s_proxy.OnBeforeWidget(direction);
            Vec3& euler = s_proxy.GetEulerProxy();

            ImGui::TableNextColumn();
            Widget_Text("direction", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragEuler("##DirLightDirectionEuler", euler))
            {
                environment.SetDirectionalLightDirection(ForwardFrom(euler));
            }

            s_proxy.OnAfterWidget();
        }

        // row: shadow cast
        {
            bool bShadow = environment.IsDirectionalLightShadowEnabled();

            ImGui::TableNextColumn();
            Widget_Text("shadow cast", true);
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##DirLightShadowCast", &bShadow))
            {
                environment.SetDirectionalLightShadowEnabled(bShadow);
            }
        }

        // row: shadow bias
        {
            float bias = environment.GetDirectionalLightShadowBias();

            ImGui::TableNextColumn();
            Widget_Text("shadow bias", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##DirLightShadowBias",bias, 0.0001f, 0.f, Limits<float>::k_max, "%.5f"))
            {
                environment.SetDirectionalLightShadowBias(bias);
            }
        }

        // cascade distances
        {
            CascadeDistancesArray distances = environment.GetDirectionalLightCascadeDistances();
            bool                  bDirty    = false;

            ImGui::TableNextColumn();
            Widget_Text("cascade distances", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            bDirty = Widget_DragFloat3("##CascadeDistances", &distances[0], 0.01f, 0.f, Limits<float>::k_max, "%.3fm");

            if (bDirty)
            {
                std::ranges::sort(distances, std::ranges::greater {});   // ensure sorted
                environment.SetDirectionalLightCascadeDistances(distances);
            }
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("image based lighting");
    if (BeginLabelTable("EnvironmentComponent_IBL"))
    {
        // row: irradiance tex
        {
            Ref<Texture> pTex = environment.GetIblIrradianceTex();
            ImGui::TableNextColumn();
            Widget_Text("irradiance texture", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_TexturePicker("##IBLIrradianceTexture", pTex))
            {
                environment.SetIblIrradianceTex(pTex);
            }
        }

        // row: irradiance tex intensity
        {
            float intensity = environment.GetIblIrradianceIntensity();

            ImGui::TableNextColumn();
            Widget_Text("irradiance intensity", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##IBLIrradianceIntensity",intensity, 0.01f, 0.f, Limits<float>::k_max))
            {
                environment.SetIblIrradianceIntensity(intensity);
            }
        }

        // row: specular tex
        {
            Ref<Texture> pTex = environment.GetIblSpecularTex();

            ImGui::TableNextColumn();
            Widget_Text("specular texture", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_TexturePicker("##IBLSpecularTexture", pTex))
            {
                environment.SetIblSpecularTex(pTex);
            }
        }

        // row: specular tex intensity
        {
            float intensity = environment.GetIblSpecularIntensity();

            ImGui::TableNextColumn();
            Widget_Text("specular intensity", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##IBLSpecularIntensity",intensity, 0.01f, 0.f, Limits<float>::k_max))
            {
                environment.SetIblSpecularIntensity(intensity);
            }
        }

        // row: BRDF LUT tex
        {
            Ref<Texture> pTex = environment.GetIblBrdfLutTex();

            ImGui::TableNextColumn();
            Widget_Text("Brdf Lut texture", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_TexturePicker("##IBLBrdfLUTTexture", pTex))
            {
                environment.SetIblBrdfLutTex(pTex);
            }
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("skybox");
    if (BeginLabelTable("EnvironmentComponent_Skybox"))
    {
        eSkyboxType type = environment.GetSkyboxType();

        // row: type
        {
            ImGui::TableNextColumn();
            Widget_Text("type", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_EnumCombo("##SkyboxType", type))
            {
                environment.SetSkyboxType(type);
            }
        }

        // row: tex
        if (type == eSkyboxType::CubeMap)
        {
            Ref<Texture> pTex = environment.GetSkyboxTex();
            ImGui::TableNextColumn();
            Widget_Text("texture", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_TexturePicker("##SkyboxTexture", pTex))
            {
                environment.SetSkyboxTex(pTex);
            }
        }

        // row: solid value
        if (type == eSkyboxType::SolidColor)
        {
            Vec3 color = environment.GetSkyboxSolidColor();
            ImGui::TableNextColumn();
            Widget_Text("solid value", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (ImGui::ColorEdit3("##SkyboxSolidColor", &color.x))
            {
                environment.SetSkyboxSolidColor(color);
            }
        }

        ImGui::EndTable();
    }

    // ssao
    ImGui::SeparatorText("ssao");
    if (BeginLabelTable("EnvironmentComponent_SSAO"))
    {
        // row: enabled
        {
            bool bEnabled = environment.IsSsaoEnabled();
            ImGui::TableNextColumn();
            Widget_Text("enabled", true);
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##SSAOEnabled", &bEnabled))
            {
                environment.SetSsaoEnabled(bEnabled);
            }
        }

        // row: radius
        {
            float radius = environment.GetSsaoRadius();
            ImGui::TableNextColumn();
            Widget_Text("radius", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##SSAORadius",radius, 0.001f, 0.f, Limits<float>::k_max, "%.4f"))
            {
                environment.SetSsaoRadius(radius);
            }
        }

        // row: bias
        {
            float bias = environment.GetSsaoBias();
            ImGui::TableNextColumn();
            Widget_Text("bias", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##SSAOBias",bias, 0.001f, 0.f, Limits<float>::k_max, "%.4f"))
            {
                environment.SetSsaoBias(bias);
            }
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("height fog");
    if (BeginLabelTable("EnvironmentComponent_Fog"))
    {
        // row: enabled
        {
            bool bEnabled = environment.IsFogEnabled();
            ImGui::TableNextColumn();
            Widget_Text("enabled", true);
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##FogEnabled", &bEnabled))
            {
                environment.SetFogEnabled(bEnabled);
            }
        }

        // row: radiance
        {
            Vec3 radiance = environment.GetFogRadiance();
            ImGui::TableNextColumn();
            Widget_Text("radiance", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (ImGui::ColorEdit3("##FogRadiance", &radiance.x))
            {
                environment.SetFogRadiance(radiance);
            }
        }

        // row: density
        {
            float density = environment.GetFogDensity();
            ImGui::TableNextColumn();
            Widget_Text("density", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##FogDensity",density, 0.0001f, 0.f, Limits<float>::k_max, "%.6f"))
            {
                environment.SetFogDensity(density);
            }
        }

        // row: fall off start
        {
            float start = environment.GetFogFallOffStart();
            ImGui::TableNextColumn();
            Widget_Text("fall off start", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##FogFallOffStart",start, 0.01f, 0.f, environment.GetFogFallOffEnd(), "%.3fm"))
            {
                environment.SetFogFallOffStart(start);
            }
        }

        // row: fall off end
        {
            float end = environment.GetFogFallOffEnd();
            ImGui::TableNextColumn();
            Widget_Text("fall off end", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##FogFallOffEnd",end, 0.01f, environment.GetFogFallOffStart(), Limits<float>::k_max, "%.3fm"))
            {
                environment.SetFogFallOffEnd(end);
            }
        }

        // row: height attenuation
        {
            float atten = environment.GetFogHeightAttenuation();
            ImGui::TableNextColumn();
            Widget_Text("height attenuation", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##FogHeightAttenuation",atten, 0.001f, 0.f, Limits<float>::k_max, "%.4f"))
            {
                environment.SetFogHeightAttenuation(atten);
            }
        }

        ImGui::EndTable();
    }

    // tonemap
    ImGui::SeparatorText("tone mapping");
    if (BeginLabelTable("EnvironmentComponent_ToneMap"))
    {
        // row: enabled
        {
            bool bEnabled = environment.IsToneMappingEnabled();
            ImGui::TableNextColumn();
            Widget_Text("enabled", true);
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##ToneMappingEnabled", &bEnabled))
            {
                environment.SetToneMappingEnabled(bEnabled);
            }
        }

        // row: type
        {
            eToneMappingType type = environment.GetToneMappingType();
            ImGui::TableNextColumn();
            Widget_Text("type", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_EnumCombo("##ToneMappingType", type))
            {
                environment.SetToneMappingType(type);
            }
        }

        // row: exposure
        {
            float exposure = environment.GetToneMappingExposure();
            ImGui::TableNextColumn();
            Widget_Text("exposure", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##ToneMappingExposure",exposure, 0.01f, 0.f, Limits<float>::k_max, "%.3f"))
            {
                environment.SetToneMappingExposure(exposure);
            }
        }

        // row: gamma
        {
            float gamma = environment.GetToneMappingGamma();
            ImGui::TableNextColumn();
            Widget_Text("gamma", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##ToneMappingGamma",gamma, 0.01f, 0.f, Limits<float>::k_max, "%.3f"))
            {
                environment.SetToneMappingGamma(gamma);
            }
        }

        ImGui::EndTable();
    }

    // bloom
    ImGui::SeparatorText("bloom");
    if (BeginLabelTable("EnvironmentComponent_Bloom"))
    {
        // row: enabled
        {
            bool bEnabled = environment.IsBloomEnabled();
            ImGui::TableNextColumn();
            Widget_Text("enabled", true);
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##BloomEnabled", &bEnabled))
            {
                environment.SetBloomEnabled(bEnabled);
            }
        }

        // row: blur radius
        {
            float radius = environment.GetBloomBlurRadius();
            ImGui::TableNextColumn();
            Widget_Text("blur radius", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##BloomBlurRadius",radius, 0.001f, 0.f, Limits<float>::k_max, "%.4f"))
            {
                environment.SetBloomBlurRadius(Max(radius, 0.f));
            }
        }

        // row: combine factor
        {
            float strength = environment.GetBloomStrength();
            ImGui::TableNextColumn();
            Widget_Text("combine factor", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragFloat("##BloomCombineFactor",strength, 0.01f, 0.f, Limits<float>::k_max))
            {
                environment.SetBloomStrength(strength);
            }
        }

        // row: number of blur passes
        {
            Int32 passes = environment.GetBloomNumBlurPasses();
            ImGui::TableNextColumn();
            Widget_Text("number of blur passes", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragInt("##BloomNumBlurPasses",passes, 0.05f, 0, environment.k_maxNumbloomPasses))
            {
                environment.SetBloomNumBlurPasses(passes);
            }
        }

        ImGui::EndTable();
    }

    // fxaa
    ImGui::SeparatorText("fxaa");
    if (BeginLabelTable("EnvironmentComponent_FXAA"))
    {
        // row: enabled
        {
            bool bEnabled = environment.IsFxaaEnabled();
            ImGui::TableNextColumn();
            Widget_Text("enabled", true);
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##FXAAEnabled", &bEnabled))
            {
                environment.SetFxaaEnabled(bEnabled);
            }
        }

        // row: quality
        {
            eFxaaQuality quality = environment.GetFxaaQuality();
            ImGui::TableNextColumn();
            Widget_Text("quality", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_EnumCombo("##FXAAQuality", quality))
            {
                environment.SetFxaaQuality(quality);
            }
        }

        ImGui::EndTable();
    }
}

}   // namespace Jam